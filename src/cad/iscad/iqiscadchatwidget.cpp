#include "iqiscadchatwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QTextBrowser>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QScrollBar>
#include <QKeyEvent>
#include <QImage>
#include <QMap>

#include "iqiscadassistantsettings.h"
#include "iqiscadchatformat.h"
#include "base/translations.h"


using namespace insight::assistant;


namespace {


QString tokens(qint64 n)
{
    if (n<1000)
        return QString::number(n);
    return QString::number(double(n)/1000., 'f', n<10000 ? 1 : 0)+"k";
}


QString quoted(const QString& text)
{
    QStringList lines=text.split('\n');
    for (auto& l: lines)
        l="> "+l;
    return lines.join('\n');
}


} // anonymous namespace




/**
 * displays the conversation, serves the images of the view_model tool
 */
class IQISCADChatWidget::Transcript
    : public QTextBrowser
{
public:
    QMap<QString, QImage> images;

    QVariant loadResource(int type, const QUrl& name) override
    {
        if (type==QTextDocument::ImageResource && name.scheme()=="assistant-image")
        {
            auto i=images.find(name.path());
            if (i!=images.end())
                return i.value();
        }
        return QTextBrowser::loadResource(type, name);
    }
};




IQISCADChatWidget::IQISCADChatWidget(ISCADAssistantAgent* agent, QWidget* parent)
    : QWidget(parent),
      agent_(agent),
      renderTimer_(new QTimer(this))
{
    auto* vbox=new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    transcript_=new Transcript;
    transcript_->setOpenExternalLinks(true);
    vbox->addWidget(transcript_, 1);

    statusLabel_=new QLabel;
    statusLabel_->setWordWrap(true);
    vbox->addWidget(statusLabel_);

    costLabel_=new QLabel;
    costLabel_->setWordWrap(true);
    costLabel_->setToolTip(_("Token usage and estimated cost (based on list prices)"));
    vbox->addWidget(costLabel_);

    input_=new QPlainTextEdit;
    input_->setPlaceholderText(
        _("Ask the assistant to create or change the model (Enter: send, Shift+Enter: new line)") );
    input_->setMaximumHeight(90);
    input_->installEventFilter(this);
    vbox->addWidget(input_);

    auto* hbox=new QHBoxLayout;
    sendBtn_=new QPushButton(_("Send"));
    stopBtn_=new QPushButton(_("Stop"));
    newBtn_=new QPushButton(_("New chat"));
    settingsBtn_=new QPushButton(_("Settings..."));
    hbox->addWidget(sendBtn_);
    hbox->addWidget(stopBtn_);
    hbox->addStretch();
    hbox->addWidget(newBtn_);
    hbox->addWidget(settingsBtn_);
    vbox->addLayout(hbox);

    renderTimer_->setSingleShot(true);
    renderTimer_->setInterval(100);
    connect(renderTimer_, &QTimer::timeout, this, &IQISCADChatWidget::render);

    connect(sendBtn_, &QPushButton::clicked, this, &IQISCADChatWidget::send);
    connect(stopBtn_, &QPushButton::clicked, agent_, &ISCADAssistantAgent::stop);
    connect(newBtn_, &QPushButton::clicked, this, &IQISCADChatWidget::newConversation);
    connect(settingsBtn_, &QPushButton::clicked, this, &IQISCADChatWidget::showSettings);

    connect(agent_, &ISCADAssistantAgent::assistantTextDelta, this,
            [this](const QString& t) { appendStreamed(Entry::Assistant, t); });
    connect(agent_, &ISCADAssistantAgent::assistantThinkingDelta, this,
            [this](const QString& t) { appendStreamed(Entry::Thinking, t); });
    connect(agent_, &ISCADAssistantAgent::assistantMessageCompleted, this,
            [this](const QString&)
            {
                // the next streamed text starts a new paragraph
                entries_.append({Entry::Info, QString()});
            });
    connect(agent_, &ISCADAssistantAgent::toolCallStarted, this,
            [this](const QString&, const QString& desc)
            {
                statusLabel_->setText(desc);
            });
    connect(agent_, &ISCADAssistantAgent::toolCallFinished, this,
            [this](const QString& name, const QString& summary, bool isError)
            {
                QString t=QString("`%1`").arg(name);
                if (name=="rebuild" || isError)
                    t+=QString(isError ? " ✗ " : " ✓ ")+summary.toHtmlEscaped();
                else
                    t+=" ✓";
                addEntry(Entry::Tool, t);
            });
    connect(agent_, &ISCADAssistantAgent::toolImageProduced, this,
            [this](const QString&, const QByteArray& data)
            {
                QImage img=QImage::fromData(data);
                if (img.isNull())
                    return;
                if (img.width()>360)
                    img=img.scaledToWidth(360, Qt::SmoothTransformation);
                QString key=QString("img%1").arg(++imageCount_);
                transcript_->images[key]=img;
                addEntry(Entry::Image, key);
            });
    connect(agent_, &ISCADAssistantAgent::errorOccurred, this,
            [this](const QString& msg) { addEntry(Entry::Error, msg); });
    connect(agent_, &ISCADAssistantAgent::contextWarning, this,
            [this](const QString& msg) { addEntry(Entry::Warning, msg); });
    connect(agent_, &ISCADAssistantAgent::statusMessage,
            statusLabel_, &QLabel::setText);
    connect(agent_, &ISCADAssistantAgent::usageUpdated,
            this, &IQISCADChatWidget::updateUsage);
    connect(agent_, &ISCADAssistantAgent::requestStarted, this,
            [this]()
            {
                requestUsage_=Usage();
                requestCost_=0;
                updateButtons();
            });
    connect(agent_, &ISCADAssistantAgent::requestFinished, this,
            [this](const QString& reason)
            {
                if (reason=="stopped")
                    addEntry(Entry::Info, _("Stopped."));
                statusLabel_->clear();
                updateButtons();
                render();
            });

    configureBackend();
    updateButtons();
    render();
}


void IQISCADChatWidget::configureBackend()
{
    auto s=IQISCADAssistantSettings::load();
    agent_->setBackend(s.createBackend());
    agent_->setSystemPrompt(s.systemPrompt());
    agent_->setDisabledTools(s.disabledTools());
    agent_->setReferenceIndex(s.referenceIndex());
    showSystemPrompt_=s.showSystemPrompt;
}


QString IQISCADChatWidget::promptDisplayText() const
{
    QStringList tools;
    for (const auto& t: agent_->tools())
        tools<<t.name;
    return QString("**%1**\n\n%2\n\n---\n\n%3 %4")
        .arg(_("System prompt:"), agent_->systemPrompt(), _("Tools:"), tools.join(", "));
}


void IQISCADChatWidget::showSettings()
{
    IQISCADAssistantSettingsDialog dlg(IQISCADAssistantSettings::load(), this);
    if (dlg.exec()==QDialog::Accepted)
    {
        QString oldPrompt=promptDisplayText();
        dlg.settings().save();
        // the conversation can be continued with the new backend
        configureBackend();
        addEntry(Entry::Info, QString(_("Using %1.")).arg(agent_->backend()->description()));
        QString newPrompt=promptDisplayText();
        if (showSystemPrompt_ && !agent_->history().isEmpty() && newPrompt!=oldPrompt)
            addEntry(Entry::Prompt, newPrompt);
    }
}


void IQISCADChatWidget::newConversation()
{
    agent_->clearConversation();
    entries_.clear();
    promptFragments_.clear();
    transcript_->images.clear();
    statusLabel_->clear();
    conversationUsage_=Usage();
    requestUsage_=Usage();
    conversationCost_=requestCost_=0;
    costLabel_->clear();
    render();
    updateButtons();
}


void IQISCADChatWidget::send()
{
    QString text=input_->toPlainText().trimmed();
    if (text.isEmpty() || agent_->isRunning())
        return;
    input_->clear();
    if (showSystemPrompt_ && agent_->history().isEmpty())
        addEntry(Entry::Prompt, promptDisplayText());
    addEntry(Entry::User, text);
    agent_->sendUserMessage(text);
}


bool IQISCADChatWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched==input_ && event->type()==QEvent::KeyPress)
    {
        auto* ke=static_cast<QKeyEvent*>(event);
        if ((ke->key()==Qt::Key_Return || ke->key()==Qt::Key_Enter)
            && !(ke->modifiers() & Qt::ShiftModifier))
        {
            send();
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}


void IQISCADChatWidget::appendStreamed(Entry::Kind kind, const QString& text)
{
    if (!entries_.isEmpty() && entries_.last().kind==kind)
        entries_.last().text+=text;
    else
        entries_.append({kind, text});
    scheduleRender();
}


void IQISCADChatWidget::addEntry(Entry::Kind kind, const QString& text)
{
    entries_.append({kind, text});
    scheduleRender();
}


void IQISCADChatWidget::scheduleRender()
{
    if (!renderTimer_->isActive())
        renderTimer_->start();
}


void IQISCADChatWidget::render()
{
    QString md;
    QList<QPair<QString, QString> > prompts; // marker, prompt text
    for (const auto& e: entries_)
    {
        if (e.text.trimmed().isEmpty())
            continue;

        switch (e.kind)
        {
        case Entry::Prompt:
        {
            // inserted after the markdown conversion (markdown has no colors and font sizes)
            QString marker=QString("@@SYSTEMPROMPT%1@@").arg(prompts.size());
            prompts.append({marker, e.text});
            md+=marker+"\n\n";
            break;
        }
        case Entry::Warning:
            md+=QString("**%1** %2\n\n").arg(_("Warning:"), e.text.toHtmlEscaped());
            break;
        case Entry::User:
            md+=QString("**%1**\n\n%2\n\n").arg(_("You:"), quoted(e.text));
            break;
        case Entry::Assistant:
            md+=e.text+"\n\n";
            break;
        case Entry::Thinking:
            md+=QString("*%1 %2*\n\n")
                    .arg(_("Thinking:"), e.text.simplified().toHtmlEscaped());
            break;
        case Entry::Tool:
            md+=e.text+"\n\n";
            break;
        case Entry::Error:
            md+=QString("**%1** %2\n\n").arg(_("Error:"), e.text.toHtmlEscaped());
            break;
        case Entry::Info:
            md+=QString("*%1*\n\n").arg(e.text);
            break;
        case Entry::Image:
            md+=QString("![view](assistant-image:%1)\n\n").arg(e.text);
            break;
        }
    }

    auto* sb=transcript_->verticalScrollBar();
    bool atEnd = sb->value() >= sb->maximum()-4;
    int oldValue=sb->value();

    transcript_->setMarkdown(md);

    for (const auto& p: prompts)
    {
        auto f=promptFragments_.find(p.second);
        if (f==promptFragments_.end())
        {
            auto pal=transcript_->palette();
            f=promptFragments_.insert(
                p.second,
                IQISCADChatFormat::subduedMarkdown(
                    p.second, transcript_->document()->defaultFont(),
                    IQISCADChatFormat::subduedColor(
                        pal.color(QPalette::Text), pal.color(QPalette::Base) ) ) );
        }
        IQISCADChatFormat::replaceMarker(transcript_->document(), p.first, f.value());
    }

    if (atEnd)
        sb->setValue(sb->maximum());
    else
        sb->setValue(oldValue);
}


void IQISCADChatWidget::updateButtons()
{
    bool running=agent_->isRunning();
    sendBtn_->setEnabled(!running);
    stopBtn_->setEnabled(running);
    settingsBtn_->setEnabled(!running);
    if (running)
        statusLabel_->setText(_("Working..."));
}


void IQISCADChatWidget::updateUsage(const Usage& sessionUsage)
{
    // usage of the last model response
    Usage delta=sessionUsage-lastSeenUsage_;
    lastSeenUsage_=sessionUsage;

    requestUsage_+=delta;
    conversationUsage_+=delta;

    auto p=agent_->backend() ? agent_->backend()->pricing() : ModelPricing();
    if (p.known)
    {
        double c=p.cost(delta);
        requestCost_+=c;
        conversationCost_+=c;
    }

    const auto& r=requestUsage_;
    QString t=QString(_("Request: %1 input, %2 cached, %3 output tokens"))
            .arg(tokens(r.inputTokens+r.cacheWriteTokens), tokens(r.cacheReadTokens), tokens(r.outputTokens));
    if (p.known)
    {
        t+=QString(", ~$%1").arg(requestCost_, 0, 'f', 2);
        t+=QString(_(" | conversation: ~$%1")).arg(conversationCost_, 0, 'f', 2);
    }
    else
    {
        t+=QString(_(" | conversation: %1 tokens")).arg(tokens(conversationUsage_.total()));
    }
    costLabel_->setText(t);
}
