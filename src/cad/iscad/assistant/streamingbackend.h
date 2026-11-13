#ifndef INSIGHT_ASSISTANT_STREAMINGBACKEND_H
#define INSIGHT_ASSISTANT_STREAMINGBACKEND_H

#include <functional>
#include <memory>

#include <QPointer>
#include <QNetworkRequest>

#include "llmbackend.h"
#include "ssestreamparser.h"

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

namespace insight {
namespace assistant {


/**
 * assembles a model response from the events of a streamed HTTP response
 */
class StreamAccumulator
{
public:
    enum Status { InProgress, Done, Error };

    std::function<void(const QString&)> onText, onThinking;

protected:
    Status status_ = InProgress;
    QString errorMessage_, errorType_;
    bool receivedContent_ = false;

    void setError(const QString& type, const QString& message);

public:
    virtual ~StreamAccumulator();

    virtual Status processEvent(const SSEStreamParser::Event& ev) =0;

    /**
     * the connection was closed: decide, whether the response is complete
     */
    virtual Status finishStream();

    virtual TurnResult result(const QString& providerId) const =0;

    inline Status status() const { return status_; }
    inline const QString& errorMessage() const { return errorMessage_; }

    /**
     * the error type of an error event, e.g. "overloaded_error"
     */
    inline const QString& errorType() const { return errorType_; }

    /**
     * true, once output has started
     * (some output might have been displayed already, the request must not be repeated)
     */
    inline bool receivedContent() const { return receivedContent_; }
};




/**
 * base class of backends, which stream the response by server-sent events.
 * Handles the HTTP request, retries, timeout and cancellation.
 */
class StreamingHttpBackend
    : public LLMBackend
{
    Q_OBJECT

    QNetworkAccessManager* nam_;
    QPointer<QNetworkReply> reply_;
    QTimer* idleTimer_;
    QTimer* retryTimer_;

    QNetworkRequest request_;
    QByteArray requestBody_;
    int attempt_ = 0;
    bool alternativeRequestUsed_ = false;
    int httpStatus_ = 0;
    QByteArray errorBody_;

    SSEStreamParser sse_;
    std::unique_ptr<StreamAccumulator> acc_;
    bool busy_ = false;

    void startRequest();
    void onReadyRead();
    void onReplyFinished();
    void onIdleTimeout();
    void complete();
    void scheduleRetry(const QString& msg, int delaySeconds);
    void abortReply();

protected:
    int idleTimeoutSeconds_ = 300;
    int maxRetries_ = 3;

    virtual std::unique_ptr<StreamAccumulator> createAccumulator() const =0;

    /**
     * stream errors (error events), after which the request may be repeated
     */
    virtual bool isRetryableStreamError(const QString& errorType) const;

    /**
     * called after a failed HTTP request (no content received yet).
     * A backend may replace the request body by an alternative request,
     * which is then sent immediately (counts as retry). Returns false otherwise.
     */
    virtual bool alternativeRequest(int httpStatus, const QByteArray& errorBody, QByteArray& requestBody);

    /**
     * start a request. Exactly one of turnFinished or turnFailed is emitted afterwards
     */
    void startTurn(const QNetworkRequest& request, const QByteArray& body);

    /**
     * report an error (asynchronously, if called from within sendTurn)
     */
    void fail(const QString& msg);
    void failLater(const QString& msg);

public:
    StreamingHttpBackend(QObject* parent=nullptr);
    ~StreamingHttpBackend();

    void cancel() override;
    bool isBusy() const override;

    /**
     * extract a readable message from an error response body
     * (understands the Anthropic and OpenAI error formats)
     */
    static QString errorMessageFromBody(int httpStatus, const QByteArray& body);

    /**
     * append a path to the path of a base URL
     */
    static QUrl endpoint(const QUrl& baseUrl, const QString& path);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_STREAMINGBACKEND_H
