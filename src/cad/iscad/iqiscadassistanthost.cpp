#include "iqiscadassistanthost.h"

#include <QTextCursor>
#include <QTextDocument>

#include "cadtypes.h"
#include "cadfeature.h"
#include "iqiscadmodelscriptedit.h"
#include "scriptpositionmap.h"
#include "cadmodel.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iscadmodelinspection.h"

#include <QBuffer>
#include <QImage>


using namespace insight::assistant;


IQISCADAssistantHost::IQISCADAssistantHost(
    IQISCADModelScriptEdit* edit,
    IQVTKCADModel3DViewer* viewer,
    QObject* parent )
    : QObject(parent),
      edit_(edit),
      viewer_(viewer),
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
    return ISCADModelInspection::commandDocumentation();
}


QStringList IQISCADAssistantHost::librarySearchPaths() const
{
    return ISCADModelInspection::librarySearchPaths();
}


QString IQISCADAssistantHost::languageReference(ReferenceLevel level)
{
    return ISCADModelInspection::languageReference(
        level==Full ? ISCADModelInspection::FullReference :
        level==Basic ? ISCADModelInspection::BasicReference :
                       ISCADModelInspection::NoReference );
}


insight::cad::ModelPtr IQISCADAssistantHost::builtModel(QString& error) const
{
    if (!edit_)
    {
        error="The editor has been closed.";
        return insight::cad::ModelPtr();
    }
    auto* sched=edit_->jobScheduler();
    if (!sched->isIdle())
    {
        // the model might be modified by the rebuild thread
        error="The model is currently being parsed or rebuilt. Try again after the rebuild has finished.";
        return insight::cad::ModelPtr();
    }
    auto pr=sched->lastParseResult();
    if (!pr || !pr->model)
    {
        error="No model has been built yet. Call rebuild first.";
        return insight::cad::ModelPtr();
    }
    return pr->model;
}


ISCADAssistantHost::SymbolTable IQISCADAssistantHost::symbols()
{
    QString error;
    auto m=builtModel(error);
    if (!m)
    {
        SymbolTable t;
        t.error=error;
        return t;
    }
    return ISCADModelInspection::symbols(*m);
}


ISCADAssistantHost::FeatureProperties IQISCADAssistantHost::featureProperties(const QString& name)
{
    QString error;
    auto m=builtModel(error);
    if (!m)
    {
        FeatureProperties p;
        p.error=error;
        return p;
    }
    return ISCADModelInspection::featureProperties(*m, name);
}


ISCADAssistantHost::Inspection IQISCADAssistantHost::captureView(
    const QString& direction,
    const QString& highlight,
    QByteArray& png )
{
    if (!viewer_)
        return {false, "No 3D view available."};

    insight::cad::FeaturePtr hf;
    if (!highlight.isEmpty())
    {
        QString error;
        auto m=builtModel(error);
        if (m)
            hf=ISCADModelInspection::builtFeature(*m, highlight, error);
        if (!hf)
            return {false, error};
    }

    arma::mat vd, up;
    const arma::mat ex=insight::vec3(1,0,0), ey=insight::vec3(0,1,0), ez=insight::vec3(0,0,1);
    if (direction=="front")       { vd=ey;  up=ez; }
    else if (direction=="back")   { vd=-ey; up=ez; }
    else if (direction=="top")    { vd=-ez; up=ey; }
    else if (direction=="bottom") { vd=ez;  up=-ey; }
    else if (direction=="left")   { vd=ex;  up=ez; }
    else if (direction=="right")  { vd=-ex; up=ez; }
    else if (direction=="iso")
    {
        vd=insight::vec3(-1, 1, -1)/std::sqrt(3.);
        up=insight::vec3(-1, 1, 2)/std::sqrt(6.); // perpendicular to vd, pointing upwards
    }

    QImage img;
    try
    {
        if (hf)
            viewer_->exposeItem(hf);
        img=viewer_->renderImage(vd, up, direction!="current");
        if (hf)
            viewer_->undoExposeItem();
    }
    catch (const std::exception& e)
    {
        return {false, QString("Rendering failed: %1").arg(QString::fromStdString(e.what()))};
    }

    if (img.isNull())
        return {false, "Rendering failed."};

    const int maxSize=1280;
    if (img.width()>maxSize || img.height()>maxSize)
        img=img.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    QBuffer buf(&png);
    buf.open(QIODevice::WriteOnly);
    img.save(&buf, "PNG");

    QString info=QString("View: %1, %2x%3 pixels.").arg(direction).arg(img.width()).arg(img.height());
    if (hf)
        info+=QString(" Feature %1 is highlighted.").arg(highlight);
    return {true, info};
}
