#ifndef INSIGHT_ASSISTANT_ANTHROPICBACKEND_H
#define INSIGHT_ASSISTANT_ANTHROPICBACKEND_H

#include <vector>

#include <QUrl>
#include <QJsonObject>

#include "streamingbackend.h"

namespace insight {
namespace assistant {


struct AnthropicConfig
{
    QString apiKey;
    QString model = "claude-opus-5-5";
    QUrl baseUrl = QUrl("https://api.anthropic.com");

    /**
     * output_config.effort: low, medium, high, xhigh, max (empty: model default)
     */
    QString effort = "high";

    int maxTokens = 64000;

    /**
     * let the API answer with another model, if the requested one declines
     * (only used with the first-party API endpoint)
     */
    bool serverSideFallback = true;

    /**
     * abort, if no data was received for this time
     */
    int idleTimeoutSeconds = 300;

    /**
     * retries on overload/rate limit/network errors before any output was received
     */
    int maxRetries = 3;
};




/**
 * assembles a model response from the events of the streaming Messages API
 */
class AnthropicStreamAccumulator
    : public StreamAccumulator
{
    std::vector<QJsonObject> blocks_;
    std::vector<QString> partialJson_;
    QString stopReason_, model_;
    Usage usage_;

    void finishBlock(size_t i);

public:
    Status processEvent(const SSEStreamParser::Event& ev) override;
    TurnResult result(const QString& providerId) const override;
};




class AnthropicBackend
    : public StreamingHttpBackend
{
    Q_OBJECT

    AnthropicConfig cfg_;

protected:
    std::unique_ptr<StreamAccumulator> createAccumulator() const override;

public:
    AnthropicBackend(const AnthropicConfig& cfg, QObject* parent=nullptr);

    QString id() const override;
    QString description() const override;
    ModelPricing pricing() const override;

    void sendTurn(
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools ) override;

    inline const AnthropicConfig& config() const { return cfg_; }

    /**
     * true, if the base URL is the first-party Anthropic API
     */
    static bool isFirstPartyEndpoint(const QUrl& baseUrl);

    /**
     * list prices of the known models
     */
    static ModelPricing pricingOf(const QString& model);

    /**
     * the JSON body of a streaming Messages API request
     */
    static QJsonObject buildRequest(
        const AnthropicConfig& cfg,
        const QString& providerId,
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools );
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_ANTHROPICBACKEND_H
