#include "iqiscadassistantsettings.h"

#include <QSettings>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

#include "assistant/anthropicbackend.h"
#include "assistant/openaicompatbackend.h"
#include "assistant/iscadassistantagent.h"
#include "iqiscadassistanthost.h"
#include "base/translations.h"


using namespace insight::assistant;


namespace {
const char* group = "assistant";
}




IQISCADAssistantSettings IQISCADAssistantSettings::load()
{
    IQISCADAssistantSettings r;
    QSettings s;
    s.beginGroup(group);
    r.provider = Provider(s.value("provider", int(Anthropic)).toInt());
    r.reference = Reference(s.value("reference", int(FullReference)).toInt());
    r.basicToolSet = s.value("basic_tool_set", false).toBool();

    AnthropicConfig ac;
    r.anthropicModel = s.value("model", ac.model).toString(); // key of phase 1
    r.anthropicEffort = s.value("effort", ac.effort).toString();
    r.anthropicApiKey = s.value("anthropic_api_key").toString();
    r.anthropicBaseUrl = s.value("anthropic_base_url").toString();

    OpenAICompatConfig oc;
    r.openaiBaseUrl = s.value("openai_base_url", oc.baseUrl.toString()).toString();
    r.openaiModel = s.value("openai_model").toString();
    r.openaiApiKey = s.value("openai_api_key").toString();
    r.openaiMaxTokens = s.value("openai_max_tokens", 0).toInt();
    r.openaiPriceInput = s.value("openai_price_input", 0.).toDouble();
    r.openaiPriceOutput = s.value("openai_price_output", 0.).toDouble();
    r.openaiPriceCachedInput = s.value("openai_price_cached_input", 0.).toDouble();
    r.openaiVision = s.value("openai_vision", false).toBool();
    return r;
}


void IQISCADAssistantSettings::save() const
{
    QSettings s;
    s.beginGroup(group);
    s.setValue("provider", int(provider));
    s.setValue("reference", int(reference));
    s.setValue("basic_tool_set", basicToolSet);

    s.setValue("model", anthropicModel);
    s.setValue("effort", anthropicEffort);
    auto setOrRemove = [&s](const char* key, const QString& v)
    {
        if (v.trimmed().isEmpty()) s.remove(key); else s.setValue(key, v.trimmed());
    };
    setOrRemove("anthropic_api_key", anthropicApiKey);
    setOrRemove("anthropic_base_url", anthropicBaseUrl);

    s.setValue("openai_base_url", openaiBaseUrl);
    s.setValue("openai_model", openaiModel);
    setOrRemove("openai_api_key", openaiApiKey);
    s.setValue("openai_max_tokens", openaiMaxTokens);
    s.setValue("openai_price_input", openaiPriceInput);
    s.setValue("openai_price_output", openaiPriceOutput);
    s.setValue("openai_price_cached_input", openaiPriceCachedInput);
    s.setValue("openai_vision", openaiVision);
}


QStringList IQISCADAssistantSettings::disabledTools() const
{
    QStringList d;
    if (basicToolSet)
        d=ISCADAssistantAgent::toolsNotIn(ISCADAssistantAgent::basicToolSet());
    if (provider==OpenAICompatible && !openaiVision && !d.contains("view_model"))
        d<<"view_model";
    return d;
}


QString IQISCADAssistantSettings::environmentApiKey() const
{
    return qEnvironmentVariable(
        provider==Anthropic ? "ANTHROPIC_API_KEY" : "OPENAI_API_KEY" );
}


LLMBackend* IQISCADAssistantSettings::createBackend() const
{
    QString envKey=environmentApiKey();

    if (provider==OpenAICompatible)
    {
        OpenAICompatConfig c;
        c.baseUrl=QUrl(openaiBaseUrl);
        c.model=openaiModel;
        c.apiKey = envKey.isEmpty() ? openaiApiKey : envKey;
        c.maxTokens=openaiMaxTokens;
        c.priceInput=openaiPriceInput;
        c.priceOutput=openaiPriceOutput;
        c.priceCachedInput=openaiPriceCachedInput;
        return new OpenAICompatBackend(c);
    }
    else
    {
        AnthropicConfig c;
        c.apiKey = envKey.isEmpty() ? anthropicApiKey : envKey;
        if (!anthropicModel.isEmpty())
            c.model=anthropicModel;
        c.effort=anthropicEffort;
        if (!anthropicBaseUrl.isEmpty())
            c.baseUrl=QUrl(anthropicBaseUrl);
        return new AnthropicBackend(c);
    }
}


QString IQISCADAssistantSettings::systemPrompt() const
{
    return ISCADAssistantAgent::defaultSystemPrompt(
        IQISCADAssistantHost::languageReference(
            reference==FullReference ? IQISCADAssistantHost::Full :
            reference==BasicReference ? IQISCADAssistantHost::Basic :
                                        IQISCADAssistantHost::None ) );
}




IQISCADAssistantSettingsDialog::IQISCADAssistantSettingsDialog(
    const IQISCADAssistantSettings& s, QWidget* parent )
    : QDialog(parent)
{
    setWindowTitle(_("Assistant settings"));
    auto* vbox=new QVBoxLayout(this);

    auto* top=new QFormLayout;
    provider_=new QComboBox;
    provider_->addItem(_("Anthropic (Claude)"));
    provider_->addItem(_("OpenAI compatible server (Ollama, vLLM, LM Studio, ...)"));
    top->addRow(_("Provider"), provider_);
    vbox->addLayout(top);

    pages_=new QStackedWidget;
    vbox->addWidget(pages_);

    // ===== Anthropic
    {
        auto* w=new QWidget;
        auto* f=new QFormLayout(w);
        f->setContentsMargins(0, 0, 0, 0);

        aModel_=new QComboBox;
        aModel_->setEditable(true);
        aModel_->addItems({"claude-opus-5-5", "claude-sonnet-5-5", "claude-haiku-5-5", "claude-fable-5-1"});
        aModel_->setCurrentText(s.anthropicModel);
        f->addRow(_("Model"), aModel_);

        aEffort_=new QComboBox;
        aEffort_->addItems({"low", "medium", "high", "xhigh", "max"});
        aEffort_->setCurrentText(s.anthropicEffort);
        f->addRow(_("Effort"), aEffort_);

        aKey_=new QLineEdit(s.anthropicApiKey);
        aKey_->setEchoMode(QLineEdit::Password);
        aKey_->setPlaceholderText(
            qEnvironmentVariableIsEmpty("ANTHROPIC_API_KEY") ?
                _("Anthropic API key (sk-ant-...)") :
                _("using ANTHROPIC_API_KEY from the environment") );
        f->addRow(_("API key"), aKey_);

        auto* note=new QLabel(
            _("API keys are created in the Anthropic Console (platform.claude.com); "
              "usage is billed per token. A Claude Pro/Max subscription does not include API usage. "
              "The environment variable ANTHROPIC_API_KEY takes precedence over the key entered here.") );
        note->setWordWrap(true);
        f->addRow(note);

        pages_->addWidget(w);
    }

    // ===== OpenAI compatible
    {
        auto* w=new QWidget;
        auto* f=new QFormLayout(w);
        f->setContentsMargins(0, 0, 0, 0);

        oPreset_=new QComboBox;
        oPreset_->addItem(_("(custom)"), QString());
        oPreset_->addItem("Ollama", "http://localhost:11434/v1");
        oPreset_->addItem("LM Studio", "http://localhost:1234/v1");
        oPreset_->addItem("vLLM", "http://localhost:8000/v1");
        oPreset_->addItem("llama.cpp server", "http://localhost:8080/v1");
        oPreset_->addItem("OpenAI", "https://api.openai.com/v1");
        f->addRow(_("Server"), oPreset_);

        oUrl_=new QLineEdit(s.openaiBaseUrl);
        f->addRow(_("Base URL"), oUrl_);

        auto* mh=new QHBoxLayout;
        oModel_=new QComboBox;
        oModel_->setEditable(true);
        oModel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        oModel_->setCurrentText(s.openaiModel);
        auto* fetch=new QPushButton(_("Fetch models"));
        mh->addWidget(oModel_);
        mh->addWidget(fetch);
        f->addRow(_("Model"), mh);
        oFetchStatus_=new QLabel;
        oFetchStatus_->setWordWrap(true);
        f->addRow(oFetchStatus_);

        oKey_=new QLineEdit(s.openaiApiKey);
        oKey_->setEchoMode(QLineEdit::Password);
        oKey_->setPlaceholderText(
            qEnvironmentVariableIsEmpty("OPENAI_API_KEY") ?
                _("optional") :
                _("using OPENAI_API_KEY from the environment") );
        f->addRow(_("API key"), oKey_);

        oMaxTokens_=new QSpinBox;
        oMaxTokens_->setRange(0, 1000000);
        oMaxTokens_->setSpecialValueText(_("server default"));
        oMaxTokens_->setValue(s.openaiMaxTokens);
        f->addRow(_("Max. output tokens"), oMaxTokens_);

        auto price = [](double v)
        {
            auto* sb=new QDoubleSpinBox;
            sb->setRange(0, 1000);
            sb->setDecimals(3);
            sb->setSuffix(" $/MTok");
            sb->setValue(v);
            return sb;
        };
        oPriceIn_=price(s.openaiPriceInput);
        oPriceOut_=price(s.openaiPriceOutput);
        oPriceCached_=price(s.openaiPriceCachedInput);
        f->addRow(_("Price input"), oPriceIn_);
        f->addRow(_("Price cached input"), oPriceCached_);
        f->addRow(_("Price output"), oPriceOut_);

        oVision_=new QCheckBox(_("The model accepts images (lets the assistant look at the 3D view)"));
        oVision_->setChecked(s.openaiVision);
        f->addRow(oVision_);

        auto* note=new QLabel(
            _("The model must support tool calling. Local models are much weaker at multi-step "
              "editing than large hosted models. Ollama uses a small context window by default, "
              "which silently cuts off the system prompt: set OLLAMA_CONTEXT_LENGTH (e.g. 32768) "
              "or choose a shorter language reference below.") );
        note->setWordWrap(true);
        f->addRow(note);

        pages_->addWidget(w);

        connect(oPreset_, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                [this](int i)
                {
                    QString u=oPreset_->itemData(i).toString();
                    if (!u.isEmpty())
                        oUrl_->setText(u);
                });
        connect(fetch, &QPushButton::clicked, this, &IQISCADAssistantSettingsDialog::fetchModels);
    }

    // ===== common
    auto* bottom=new QFormLayout;
    reference_=new QComboBox;
    reference_->addItem(_("Full language and command reference (~20k tokens)"));
    reference_->addItem(_("Language reference without command list (~10k tokens)"));
    reference_->addItem(_("None (command_help tool only)"));
    reference_->setCurrentIndex(int(s.reference));
    bottom->addRow(_("System prompt"), reference_);
    toolSet_=new QComboBox;
    toolSet_->addItem(_("All tools"));
    toolSet_->addItem(_("Basic tools (read, edit, rebuild, help, inspect) for small models"));
    toolSet_->setCurrentIndex(s.basicToolSet ? 1 : 0);
    bottom->addRow(_("Tools"), toolSet_);
    vbox->addLayout(bottom);

    auto* bb=new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    connect(bb, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(bb, &QDialogButtonBox::rejected, this, &QDialog::reject);
    vbox->addWidget(bb);

    connect(provider_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            pages_, &QStackedWidget::setCurrentIndex);
    provider_->setCurrentIndex(int(s.provider));
    pages_->setCurrentIndex(int(s.provider));
}


void IQISCADAssistantSettingsDialog::fetchModels()
{
    oFetchStatus_->setText(_("Fetching model list..."));

    QNetworkAccessManager nam;
    QNetworkRequest req(insight::assistant::StreamingHttpBackend::endpoint(
        QUrl(oUrl_->text().trimmed()), "/models" ));
    QString key=qEnvironmentVariable("OPENAI_API_KEY");
    if (key.isEmpty())
        key=oKey_->text().trimmed();
    if (!key.isEmpty())
        req.setRawHeader("Authorization", "Bearer "+key.toUtf8());

    auto* reply=nam.get(req);
    QEventLoop loop;
    QTimer timeout;
    timeout.setSingleShot(true);
    connect(&timeout, &QTimer::timeout, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timeout.start(10000);
    loop.exec();

    if (!reply->isFinished())
    {
        reply->abort();
        oFetchStatus_->setText(_("No answer from the server."));
    }
    else if (reply->error()!=QNetworkReply::NoError)
    {
        oFetchStatus_->setText(QString(_("Error: %1")).arg(reply->errorString()));
    }
    else
    {
        auto ids=OpenAICompatBackend::parseModelList(reply->readAll());
        QString cur=oModel_->currentText();
        oModel_->clear();
        oModel_->addItems(ids);
        oModel_->setCurrentText(cur.isEmpty() && !ids.isEmpty() ? ids.first() : cur);
        oFetchStatus_->setText(QString(_("%1 models available.")).arg(ids.size()));
    }
    reply->deleteLater();
}


IQISCADAssistantSettings IQISCADAssistantSettingsDialog::settings() const
{
    IQISCADAssistantSettings s;
    s.provider=IQISCADAssistantSettings::Provider(provider_->currentIndex());
    s.reference=IQISCADAssistantSettings::Reference(reference_->currentIndex());
    s.basicToolSet = toolSet_->currentIndex()==1;

    s.anthropicModel=aModel_->currentText().trimmed();
    s.anthropicEffort=aEffort_->currentText();
    s.anthropicApiKey=aKey_->text().trimmed();
    s.anthropicBaseUrl=IQISCADAssistantSettings::load().anthropicBaseUrl; // not editable here

    s.openaiBaseUrl=oUrl_->text().trimmed();
    s.openaiModel=oModel_->currentText().trimmed();
    s.openaiApiKey=oKey_->text().trimmed();
    s.openaiMaxTokens=oMaxTokens_->value();
    s.openaiPriceInput=oPriceIn_->value();
    s.openaiPriceOutput=oPriceOut_->value();
    s.openaiPriceCachedInput=oPriceCached_->value();
    s.openaiVision=oVision_->isChecked();
    return s;
}
