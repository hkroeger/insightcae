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
#include <QSettings>
#include <QDialog>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>

#include "assistant/anthropicbackend.h"
#include "base/translations.h"


using namespace insight::assistant;


namespace {


const char* settingsModel = "assistant/model";
const char* settingsEffort = "assistant/effort";
const char* settingsApiKey = "assistant/anthropic_api_key";
const char* settingsBaseUrl = "assistant/anthropic_base_url";


/**
 * API prices in USD per million tokens: input, output, cache read
 * (cache write: 1.25 x input)
 */
struct Price { double in, out, cacheRead; };

bool priceOf(const QString& model, Price& p)
{
    static const std::map<QString, Price> prices{
        {"claude-fable-5-1", {10., 50., 0.25}},
        {"claude-opus-5-5", {4., 20., 0.2}},
        {"claude-sonnet-5-5", {2., 10., 0.2}},
        {"claude-haiku-5-5", {0.1, 0.5, 0.01}}
    };
    auto i=prices.find(model);
    if (i==prices.end())
        return false;
    p=i->second;
    return true;
}


QString quoted(const QString& text)
{
    QStringList lines=text.split('\n');
    for (auto& l: lines)
        l="> "+l;
    return lines.join('\n');
}


} // anonymous namespace




IQISCADChatWidget::IQISCADChatWidget(ISCADAssistantAgent* agent, QWidget* parent)
    : QWidget(parent),
      agent_(agent),
      renderTimer_(new QTimer(this))
{
    auto* vbox=new QVBoxLayout(this);
    vbox->setContentsMargins(0, 0, 0, 0);

    transcript_=new QTextBrowser;
    transcript_->setOpenExternalLinks(true);
    vbox->addWidget(transcript_, 1);

    statusLabel_=new QLabel;
    statusLabel_->setWordWrap(true);
    vbox->addWidget(statusLabel_);

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
    connect(agent_, &ISCADAssistantAgent::errorOccurred, this,
            [this](const QString& msg) { addEntry(Entry::Error, msg); });
    connect(agent_, &ISCADAssistantAgent::statusMessage,
            statusLabel_, &QLabel::setText);
    connect(agent_, &ISCADAssistantAgent::usageUpdated,
            this, &IQISCADChatWidget::updateUsage);
    connect(agent_, &ISCADAssistantAgent::requestStarted,
            this, &IQISCADChatWidget::updateButtons);
    connect(agent_, &ISCADAssistantAgent::requestFinished, this,
            [this](const QString& reason)
            {
                if (reason=="stopped")
                    addEntry(Entry::Info, _("Stopped."));
                statusLabel_->clear();
                updateUsage(agent_->sessionUsage());
                updateButtons();
                render();
            });

    configureBackend();
    updateButtons();
    render();
}


void IQISCADChatWidget::configureBackend()
{
    QSettings s;
    AnthropicConfig cfg;
    cfg.apiKey=qEnvironmentVariable("ANTHROPIC_API_KEY");
    if (cfg.apiKey.isEmpty())
        cfg.apiKey=s.value(settingsApiKey).toString();
    cfg.model=s.value(settingsModel, cfg.model).toString();
    cfg.effort=s.value(settingsEffort, cfg.effort).toString();
    QString baseUrl=s.value(settingsBaseUrl).toString();
    if (!baseUrl.isEmpty())
        cfg.baseUrl=QUrl(baseUrl);

    agent_->setBackend(new AnthropicBackend(cfg));
}


void IQISCADChatWidget::showSettings()
{
    QSettings s;

    QDialog dlg(this);
    dlg.setWindowTitle(_("Assistant settings"));
    auto* form=new QFormLayout(&dlg);

    auto* model=new QComboBox;
    model->setEditable(true);
    model->addItems({"claude-opus-5-5", "claude-sonnet-5-5", "claude-haiku-5-5", "claude-fable-5-1"});
    model->setCurrentText(s.value(settingsModel, AnthropicConfig().model).toString());
    form->addRow(_("Model"), model);

    auto* effort=new QComboBox;
    effort->addItems({"low", "medium", "high", "xhigh", "max"});
    effort->setCurrentText(s.value(settingsEffort, AnthropicConfig().effort).toString());
    form->addRow(_("Effort"), effort);

    auto* key=new QLineEdit;
    key->setEchoMode(QLineEdit::Password);
    key->setText(s.value(settingsApiKey).toString());
    bool envKey=!qEnvironmentVariable("ANTHROPIC_API_KEY").isEmpty();
    key->setPlaceholderText(
        envKey ? _("using ANTHROPIC_API_KEY from the environment")
               : _("Anthropic API key (sk-ant-...)") );
    form->addRow(_("API key"), key);

    auto* note=new QLabel(
        _("The key is stored unencrypted in the user settings. "
          "The environment variable ANTHROPIC_API_KEY takes precedence.\n"
          "API keys are created in the Anthropic Console (platform.claude.com); "
          "usage is billed per token. A Claude Pro/Max subscription does not include API usage.") );
    note->setWordWrap(true);
    form->addRow(note);

    auto* bb=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(bb);

    if (dlg.exec()==QDialog::Accepted)
    {
        s.setValue(settingsModel, model->currentText().trimmed());
        s.setValue(settingsEffort, effort->currentText());
        if (key->text().trimmed().isEmpty())
            s.remove(settingsApiKey);
        else
            s.setValue(settingsApiKey, key->text().trimmed());

        // the conversation can be continued with the new backend
        configureBackend();
        addEntry(Entry::Info, QString(_("Using %1.")).arg(agent_->backend()->description()));
    }
}


void IQISCADChatWidget::newConversation()
{
    agent_->clearConversation();
    entries_.clear();
    statusLabel_->clear();
    render();
    updateButtons();
}


void IQISCADChatWidget::send()
{
    QString text=input_->toPlainText().trimmed();
    if (text.isEmpty() || agent_->isRunning())
        return;
    input_->clear();
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
    for (const auto& e: entries_)
    {
        if (e.text.trimmed().isEmpty())
            continue;

        switch (e.kind)
        {
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
        }
    }

    auto* sb=transcript_->verticalScrollBar();
    bool atEnd = sb->value() >= sb->maximum()-4;
    int oldValue=sb->value();

    transcript_->setMarkdown(md);

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


void IQISCADChatWidget::updateUsage(const Usage& u)
{
    if (agent_->isRunning())
        return; // the status line shows the progress

    if (u.inputTokens+u.outputTokens+u.cacheReadTokens+u.cacheWriteTokens==0)
        return;

    QString t=QString(_("Session: %1 input, %2 cached, %3 output tokens"))
            .arg(u.inputTokens+u.cacheWriteTokens)
            .arg(u.cacheReadTokens)
            .arg(u.outputTokens);

    Price p;
    auto* ab=qobject_cast<AnthropicBackend*>(agent_->backend());
    if (ab && priceOf(ab->config().model, p))
    {
        double cost = 1e-6*(
            p.in*u.inputTokens
            + 1.25*p.in*u.cacheWriteTokens
            + p.cacheRead*u.cacheReadTokens
            + p.out*u.outputTokens );
        t+=QString(_(", approx. $%1")).arg(cost, 0, 'f', 2);
    }
    statusLabel_->setText(t);
}
