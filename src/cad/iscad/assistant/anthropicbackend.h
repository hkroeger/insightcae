#ifndef INSIGHT_ASSISTANT_ANTHROPICBACKEND_H
#define INSIGHT_ASSISTANT_ANTHROPICBACKEND_H

#include <functional>
#include <memory>
#include <vector>

#include <QPointer>
#include <QUrl>
#include <QJsonObject>

#include "llmbackend.h"
#include "ssestreamparser.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

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
{
public:
    enum Status { InProgress, Done, Error };

    std::function<void(const QString&)> onText, onThinking;

private:
    std::vector<QJsonObject> blocks_;
    std::vector<QString> partialJson_;
    QString stopReason_, model_, errorMessage_, errorType_;
    Usage usage_;
    Status status_ = InProgress;
    bool receivedContent_ = false;

    void finishBlock(size_t i);

public:
    Status processEvent(const SSEStreamParser::Event& ev);

    inline Status status() const { return status_; }
    inline const QString& errorMessage() const { return errorMessage_; }

    /**
     * the API error type of an error event, e.g. "overloaded_error"
     */
    inline const QString& errorType() const { return errorType_; }

    /**
     * true, once a content block has been started
     * (some output might have been displayed already)
     */
    inline bool receivedContent() const { return receivedContent_; }

    TurnResult result(const QString& providerId) const;
};




class AnthropicBackend
    : public LLMBackend
{
    Q_OBJECT

    AnthropicConfig cfg_;
    QNetworkAccessManager* nam_;
    QPointer<QNetworkReply> reply_;
    QTimer* idleTimer_;
    QTimer* retryTimer_;

    QByteArray requestBody_;
    int attempt_ = 0;
    int httpStatus_ = 0;
    QByteArray errorBody_;

    SSEStreamParser sse_;
    std::unique_ptr<AnthropicStreamAccumulator> acc_;
    bool busy_ = false;

    void startRequest();
    void onReadyRead();
    void onReplyFinished();
    void onIdleTimeout();
    void fail(const QString& msg);
    void abortReply();

public:
    AnthropicBackend(const AnthropicConfig& cfg, QObject* parent=nullptr);
    ~AnthropicBackend();

    QString id() const override;
    QString description() const override;

    void sendTurn(
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools ) override;

    void cancel() override;
    bool isBusy() const override;

    inline const AnthropicConfig& config() const { return cfg_; }

    /**
     * true, if the base URL is the first-party Anthropic API
     */
    static bool isFirstPartyEndpoint(const QUrl& baseUrl);

    /**
     * the JSON body of a streaming Messages API request
     */
    static QJsonObject buildRequest(
        const AnthropicConfig& cfg,
        const QString& providerId,
        const QString& systemPrompt,
        const QList<ChatMessage>& history,
        const QList<ToolDefinition>& tools );

    /**
     * extract a readable message from an API error response body
     */
    static QString errorMessageFromBody(int httpStatus, const QByteArray& body);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_ANTHROPICBACKEND_H
