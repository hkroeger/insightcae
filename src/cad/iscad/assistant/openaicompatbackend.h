#ifndef INSIGHT_ASSISTANT_OPENAICOMPATBACKEND_H
#define INSIGHT_ASSISTANT_OPENAICOMPATBACKEND_H

#include <map>

#include <QUrl>
#include <QJsonObject>

#include "streamingbackend.h"

namespace insight {
namespace assistant {


/**
 * a server with OpenAI compatible chat completions endpoint
 * (e.g. Ollama, vLLM, LM Studio, llama.cpp server, or a hosted service)
 */
struct OpenAICompatConfig
{
    /**
     * including the version path, e.g. http://localhost:11434/v1
     */
    QUrl baseUrl = QUrl("http://localhost:11434/v1");

    /**
     * optional (sent as bearer token)
     */
    QString apiKey;

    QString model;

    /**
     * maximum output tokens, 0: server default
     */
    int maxTokens = 0;

    /**
     * request token usage in the stream (stream_options.include_usage)
     */
    bool streamUsage = true;

    int idleTimeoutSeconds = 600;
    int maxRetries = 2;

    /**
     * prices in USD per million tokens (all zero: no cost display)
     */
    double priceInput = 0, priceOutput = 0, priceCachedInput = 0;
};




/**
 * assembles a model response from a streamed chat completion
 */
class OpenAICompatStreamAccumulator
    : public StreamAccumulator
{
    struct PartialToolCall
    {
        QString id, name, arguments;
    };

    QString text_, finishReason_, model_;
    std::map<int, PartialToolCall> toolCalls_;
    int lastToolIndex_ = -1;
    Usage usage_;
    bool finished_ = false;

public:
    Status processEvent(const SSEStreamParser::Event& ev) override;
    Status finishStream() override;
    TurnResult result(const QString& providerId) const override;
};




class OpenAICompatBackend
    : public StreamingHttpBackend
{
    Q_OBJECT

    OpenAICompatConfig cfg_;

protected:
    std::unique_ptr<StreamAccumulator> createAccumulator() const override;
    bool isRetryableStreamError(const QString& errorType) const override;

public:
    OpenAICompatBackend(const OpenAICompatConfig& cfg, QObject* parent=nullptr);

    QString id() const override;
    QString description() const override;
    ModelPricing pricing() const override;

    void sendTurn(
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools ) override;

    inline const OpenAICompatConfig& config() const { return cfg_; }

    static QJsonObject buildRequest(
        const OpenAICompatConfig& cfg,
        const QString& providerId,
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools );

    /**
     * the model ids from the response of GET <baseUrl>/models
     */
    static QStringList parseModelList(const QByteArray& response);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_OPENAICOMPATBACKEND_H
