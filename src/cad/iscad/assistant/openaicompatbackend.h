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
     * context window of the model on the server in tokens (0: unknown).
     * Ollama: OLLAMA_CONTEXT_LENGTH, shown by "ollama ps"
     */
    int contextLength = 0;

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

    /**
     * the tools of the request: for typing the parameters of tool calls in the text
     */
    QList<ToolDefinition> tools_;

public:
    OpenAICompatStreamAccumulator(const QList<ToolDefinition>& tools=QList<ToolDefinition>());


    Status processEvent(const SSEStreamParser::Event& ev) override;
    Status finishStream() override;
    TurnResult result(const QString& providerId) const override;
};




/**
 * a tool call, which the model wrote into the text instead of the tool_calls field
 */
struct TextToolCall
{
    QString name;
    QJsonObject input;
};


/**
 * extracts tool calls in the XML format of Qwen3-Coder from text:
 * <tool_call><function=NAME><parameter=P>value</parameter>...</function></tool_call>.
 * Some local servers (e.g. Ollama) fail to recognize them, if the model omits a tag.
 * The parameter values are converted according to the input schemas of the tools.
 * Returns the extracted calls; the calls and stray tool_call tags are removed from text.
 */
QList<TextToolCall> extractTextToolCalls(QString& text, const QList<ToolDefinition>& tools);


/**
 * describes the tools and the XML tool call format in text form
 * (for requests without the tools field)
 */
QString textToolInstructions(const QList<ToolDefinition>& tools);




class OpenAICompatBackend
    : public StreamingHttpBackend
{
    Q_OBJECT

    OpenAICompatConfig cfg_;

    /**
     * request of the current turn
     */
    QString systemPrompt_;
    QList<ChatMessage> history_;
    QList<ToolDefinition> tools_;
    bool toolsInText_ = false;

protected:
    std::unique_ptr<StreamAccumulator> createAccumulator() const override;
    bool isRetryableStreamError(const QString& errorType) const override;

    /**
     * Ollama fails with HTTP 500 ("XML syntax error"), if its tool call parser
     * cannot read the output of the model. The request is then repeated without
     * the tools field (tools described in the system prompt instead): the server
     * returns the raw text and the tool calls are extracted from it.
     */
    bool alternativeRequest(int httpStatus, const QByteArray& errorBody, QByteArray& requestBody) override;

public:
    OpenAICompatBackend(const OpenAICompatConfig& cfg, QObject* parent=nullptr);

    QString id() const override;
    QString description() const override;
    ModelPricing pricing() const override;
    int contextLength() const override { return cfg_.contextLength; }

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
        const QList<ToolDefinition>& tools,
        bool toolsInText=false );

    /**
     * the model ids from the response of GET <baseUrl>/models
     */
    static QStringList parseModelList(const QByteArray& response);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_OPENAICOMPATBACKEND_H
