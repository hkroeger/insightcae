#ifndef IQISCADASSISTANTSETTINGS_H
#define IQISCADASSISTANTSETTINGS_H

#include <QDialog>

namespace insight { namespace assistant { class LLMBackend; } }

class QComboBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QStackedWidget;
class QLabel;


/**
 * configuration of the chat assistant (stored in the user settings)
 */
struct IQISCADAssistantSettings
{
    enum Provider { Anthropic=0, OpenAICompatible=1 };
    Provider provider = Anthropic;

    /**
     * how much of the language documentation is put into the system prompt
     */
    enum Reference { FullReference=0, BasicReference=1, NoReference=2 };
    Reference reference = FullReference;

    // ===== Anthropic
    QString anthropicModel;
    QString anthropicEffort;
    QString anthropicApiKey;
    QString anthropicBaseUrl;

    // ===== OpenAI compatible server
    QString openaiBaseUrl;
    QString openaiModel;
    QString openaiApiKey;
    int openaiMaxTokens = 0;
    double openaiPriceInput = 0, openaiPriceOutput = 0, openaiPriceCachedInput = 0;

    static IQISCADAssistantSettings load();
    void save() const;

    /**
     * the API key from the environment (ANTHROPIC_API_KEY or OPENAI_API_KEY), if set
     */
    QString environmentApiKey() const;

    insight::assistant::LLMBackend* createBackend() const;
    QString systemPrompt() const;
};




class IQISCADAssistantSettingsDialog
    : public QDialog
{
    Q_OBJECT

    QComboBox* provider_;
    QStackedWidget* pages_;
    QComboBox* reference_;

    QComboBox *aModel_, *aEffort_;
    QLineEdit* aKey_;

    QComboBox* oPreset_;
    QLineEdit *oUrl_, *oKey_;
    QComboBox* oModel_;
    QLabel* oFetchStatus_;
    QSpinBox* oMaxTokens_;
    QDoubleSpinBox *oPriceIn_, *oPriceOut_, *oPriceCached_;

    void fetchModels();

public:
    IQISCADAssistantSettingsDialog(const IQISCADAssistantSettings& s, QWidget* parent=nullptr);

    IQISCADAssistantSettings settings() const;
};

#endif // IQISCADASSISTANTSETTINGS_H
