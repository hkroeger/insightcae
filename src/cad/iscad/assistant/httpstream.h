#ifndef INSIGHT_ASSISTANT_HTTPSTREAM_H
#define INSIGHT_ASSISTANT_HTTPSTREAM_H

#include <memory>

#include <QObject>
#include <QByteArray>
#include <QNetworkRequest>

namespace insight {
namespace assistant {


/**
 * a single HTTP(S) request with a streamed response, performed by libcurl in a worker thread.
 *
 * Qt's own SSL backend is not used: the Qt build of the dependencies expects OpenSSL 1.1
 * and fails the TLS handshake with OpenSSL 3 (missing peer certificate function).
 * libcurl uses the system's TLS library, CA store and proxy variables (https_proxy, no_proxy).
 *
 * All signals are delivered in the thread of the object (the GUI thread).
 * After finished, no further signals are emitted.
 */
class HttpStream
    : public QObject
{
    Q_OBJECT

public:
    enum Error { NoError, Canceled, ConnectionRefused, HostNotFound, Timeout, OtherError };
    Q_ENUM(Error)

    struct State;

private:
    std::shared_ptr<State> state_;
    bool finished_ = false;

    HttpStream(QObject* parent);

public:
    ~HttpStream();

    /**
     * start a request (method: "GET" or "POST"; the URL and raw headers are taken from req)
     */
    static HttpStream* start(
        const QByteArray& method,
        const QNetworkRequest& req,
        const QByteArray& body,
        QObject* parent = nullptr );

    /**
     * stop the transfer. finished is not emitted anymore.
     */
    void abort();

    inline bool isFinished() const { return finished_; }

Q_SIGNALS:
    /**
     * the response header has been received
     */
    void responseStarted(int httpStatus, const QByteArray& retryAfter);
    void dataReceived(const QByteArray& data);
    void finished(insight::assistant::HttpStream::Error error, const QString& message);
};


} // namespace assistant
} // namespace insight

#endif // INSIGHT_ASSISTANT_HTTPSTREAM_H
