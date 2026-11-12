#include "iqiscadassistanthost.h"

#include <QFile>
#include <QTextCursor>
#include <QTextDocument>

#include "cadtypes.h"
#include "cadfeature.h"
#include "iqiscadmodelscriptedit.h"
#include "scriptpositionmap.h"


using namespace insight::assistant;


IQISCADAssistantHost::IQISCADAssistantHost(IQISCADModelScriptEdit* edit, QObject* parent)
    : QObject(parent),
      edit_(edit),
      launchFailureCheck_(new QTimer(this))
{
    auto* sched = edit_->jobScheduler();

    connect(sched, &IQISCADScriptJobScheduler::scriptError, this,
            [this](long failpos, QString errorMsg, int, IQISCADScriptModelGenerator::Task task)
            {
                onScriptError(failpos, errorMsg, int(task));
            });
    connect(sched, &IQISCADScriptJobScheduler::jobEnded, this,
            [this](IQISCADScriptModelGenerator::Task task)
            {
                onJobEnded(int(task));
            });

    // a job, which failed to launch, never ends:
    // complete the rebuild, once the scheduler is idle
    launchFailureCheck_->setSingleShot(true);
    launchFailureCheck_->setInterval(2000);
    connect(launchFailureCheck_, &QTimer::timeout, this, [this]()
            {
                if (!rebuildDone_ || !edit_)
                    return;
                if (edit_->jobScheduler()->isIdle())
                    completeRebuild();
                else
                    launchFailureCheck_->start();
            });
}


QString IQISCADAssistantHost::scriptText() const
{
    return edit_ ? edit_->toPlainText() : QString();
}


QString IQISCADAssistantHost::scriptFileName() const
{
    if (!edit_)
        return QString();
    return QString::fromStdString(edit_->filename().filename().string());
}


void IQISCADAssistantHost::beginAssistantTurn()
{
    firstEditInTurn_=true;
    if (edit_)
        edit_->setAssistantActive(true);
}


void IQISCADAssistantHost::endAssistantTurn()
{
    if (edit_)
        edit_->setAssistantActive(false);
}


void IQISCADAssistantHost::replaceText(int pos, int length, const QString& text)
{
    if (!edit_)
        return;

    QTextCursor c(edit_->document());
    c.setPosition(pos);
    c.setPosition(pos+length, QTextCursor::KeepAnchor);

    if (firstEditInTurn_)
    {
        c.beginEditBlock();
        firstEditInTurn_=false;
    }
    else
    {
        c.joinPreviousEditBlock();
    }
    c.insertText(text);
    c.endEditBlock();

    // show the change
    QTextCursor vc(edit_->document());
    vc.setPosition(pos+text.size());
    edit_->setTextCursor(vc);
    edit_->ensureCursorVisible();
}


void IQISCADAssistantHost::rebuild(int upToPos, RebuildCallback done)
{
    if (!edit_)
    {
        done(false, "The editor has been closed.", -1);
        return;
    }

    if (rebuildDone_)
        cancelRebuild();

    rebuildDone_=done;
    rebuildFailed_=false;
    rebuildError_.clear();
    rebuildErrorPos_=-1;

    QString text=edit_->toPlainText();
    if (upToPos>=0 && upToPos<text.size())
        text=text.left(upToPos);
    rebuildScript_=text.toStdString();

    // rebuild without the postprocessing actions, which write files
    if (upToPos>=0)
    {
        edit_->jobScheduler()->rebuildScript(
            rebuildScript_, IQISCADScriptModelGenerator::Rebuild );
    }
    else
    {
        edit_->jobScheduler()->rebuild(IQISCADScriptModelGenerator::Rebuild);
    }
}


void IQISCADAssistantHost::onScriptError(long failpos, const QString& errorMsg, int task)
{
    if (!rebuildDone_ || task<IQISCADScriptModelGenerator::Rebuild)
        return;

    rebuildFailed_=true;
    if (!rebuildError_.isEmpty())
        rebuildError_+="\n";
    rebuildError_+=errorMsg;
    if (failpos>=0 && rebuildErrorPos_<0)
    {
        // failpos is a byte offset in the parsed script
        rebuildErrorPos_=ScriptPositionMap(rebuildScript_).toChar(failpos);
    }

    launchFailureCheck_->start();
}


void IQISCADAssistantHost::onJobEnded(int task)
{
    if (!rebuildDone_ || task<IQISCADScriptModelGenerator::Rebuild)
        return;
    completeRebuild();
}


void IQISCADAssistantHost::completeRebuild()
{
    launchFailureCheck_->stop();
    if (!rebuildDone_)
        return;

    auto done=rebuildDone_;
    rebuildDone_=nullptr;
    done(!rebuildFailed_, rebuildError_, rebuildErrorPos_);
}


void IQISCADAssistantHost::cancelRebuild()
{
    launchFailureCheck_->stop();
    if (rebuildDone_)
    {
        rebuildDone_=nullptr;
        if (edit_)
            edit_->onCancelRebuild();
    }
}


QList<CommandDoc> IQISCADAssistantHost::commandDocumentation() const
{
    QList<CommandDoc> docs;
    if (insight::cad::Feature::ruleDocumentationFunctions_)
    {
        for (const auto& rd: *insight::cad::Feature::ruleDocumentationFunctions_)
        {
            for (const auto& info: rd.second())
            {
                docs.append({
                    QString::fromStdString(info.command_),
                    QString::fromStdString(info.signature_),
                    QString::fromStdString(info.documentation_)
                });
            }
        }
    }
    return docs;
}


QStringList IQISCADAssistantHost::librarySearchPaths() const
{
    QStringList paths;
    insight::cad::sharedModelLocations locs;
    for (const auto& p: locs)
    {
        if (p==boost::filesystem::path("."))
            continue;
        paths<<QString::fromStdString(p.string());
    }
    return paths;
}


QString IQISCADAssistantHost::languageReference()
{
    QString ref;
    for (const char* fn: {":/iscad_assistant/insightcad.md", ":/iscad_assistant/iscad-reference.md"})
    {
        QFile f(fn);
        if (f.open(QIODevice::ReadOnly|QIODevice::Text))
        {
            if (!ref.isEmpty())
                ref+="\n\n";
            ref+=QString::fromUtf8(f.readAll());
        }
    }
    return ref;
}
