#include "streamingbackend.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>

namespace insight {
namespace assistant {




// ================================================================
// StreamAccumulator
// ================================================================


StreamAccumulator::~StreamAccumulator()
{}


void StreamAccumulator::setError(const QString& type, const QString& message)
{
    status_=Error;
    errorType_=type;
    errorMessage_ = type.isEmpty() ? message : QString("%1: %2").arg(type, message);
}


StreamAccumulator::Status StreamAccumulator::finishStream()
{
    return status_;
}




// ================================================================
// StreamingHttpBackend
// ================================================================


StreamingHttpBackend::StreamingHttpBackend(QObject* parent)
    : LLMBackend(parent),
      idleTimer_(new QTimer(this)),
      retryTimer_(new QTimer(this))
{
    idleTimer_->setSingleShot(true);
    connect(idleTimer_, &QTimer::timeout, this, &StreamingHttpBackend::onIdleTimeout);
    retryTimer_->setSingleShot(true);
    connect(retryTimer_, &QTimer::timeout, this, &StreamingHttpBackend::startRequest);
}


StreamingHttpBackend::~StreamingHttpBackend()
{
    abortReply();
}


QUrl StreamingHttpBackend::endpoint(const QUrl& baseUrl, const QString& path)
{
    QUrl url=baseUrl;
    QString p=url.path();
    while (p.endsWith('/'))
        p.chop(1);
    url.setPath(p+path);
    return url;
}


QString StreamingHttpBackend::errorMessageFromBody(int httpStatus, const QByteArray& body)
{
    QJsonParseError err;
    auto doc=QJsonDocument::fromJson(body, &err);
    if (err.error==QJsonParseError::NoError && doc.isObject())
    {
        auto ev=doc.object().value("error");
        if (ev.isObject())
        {
            auto e=ev.toObject();
            QString type=e.value("type").toString();
            QString msg=e.value("message").toString();
            if (type.isEmpty())
                return QString("HTTP %1: %2").arg(httpStatus).arg(msg);
            return QString("HTTP %1, %2: %3").arg(httpStatus).arg(type, msg);
        }
        else if (ev.isString())
        {
            return QString("HTTP %1: %2").arg(httpStatus).arg(ev.toString());
        }
    }
    QString b=QString::fromUtf8(body.left(500)).trimmed();
    if (b.isEmpty())
        return QString("HTTP %1").arg(httpStatus);
    return QString("HTTP %1: %2").arg(httpStatus).arg(b);
}


bool StreamingHttpBackend::alternativeRequest(int, const QByteArray&, QByteArray&)
{
    return false;
}


bool StreamingHttpBackend::isRetryableStreamError(const QString& errorType) const
{
    static const QStringList retryable{"overloaded_error", "api_error", "rate_limit_error"};
    return retryable.contains(errorType);
}


void StreamingHttpBackend::startTurn(const QNetworkRequest& request, const QByteArray& body)
{
    cancel();
    busy_=true;
    attempt_=0;
    alternativeRequestUsed_=false;
    request_=request;
    requestBody_=body;
    startRequest();
}


void StreamingHttpBackend::startRequest()
{
    if (!busy_)
        return;

    httpStatus_=0;
    retryAfter_.clear();
    errorBody_.clear();
    sse_.reset();
    acc_=createAccumulator();
    acc_->onText = [this](const QString& t) { Q_EMIT textDelta(t); };
    acc_->onThinking = [this](const QString& t) { Q_EMIT thinkingDelta(t); };

    reply_=HttpStream::start("POST", request_, requestBody_, this);
    connect(reply_, &HttpStream::responseStarted, this, &StreamingHttpBackend::onResponseStarted);
    connect(reply_, &HttpStream::dataReceived, this, &StreamingHttpBackend::onDataReceived);
    connect(reply_, &HttpStream::finished, this, &StreamingHttpBackend::onReplyFinished);

    idleTimer_->start(idleTimeoutSeconds_*1000);
}


void StreamingHttpBackend::complete()
{
    auto result=acc_->result(id());
    result.alternativeRequest=alternativeRequestUsed_;
    abortReply();
    busy_=false;
    Q_EMIT turnFinished(result);
}


void StreamingHttpBackend::scheduleRetry(const QString& msg, int delaySeconds)
{
    abortReply();
    ++attempt_;
    Q_EMIT statusMessage(QString("%1 - retrying in %2 s").arg(msg).arg(delaySeconds));
    retryTimer_->start(delaySeconds*1000);
}


void StreamingHttpBackend::onResponseStarted(int httpStatus, const QByteArray& retryAfter)
{
    if (!reply_ || !busy_)
        return;
    idleTimer_->start(idleTimeoutSeconds_*1000);
    httpStatus_=httpStatus;
    retryAfter_=retryAfter;
}


void StreamingHttpBackend::onDataReceived(const QByteArray& data)
{
    if (!reply_ || !busy_)
        return;

    idleTimer_->start(idleTimeoutSeconds_*1000);

    if (httpStatus_!=200)
    {
        errorBody_.append(data);
        return;
    }

    for (const auto& ev: sse_.feed(data))
    {
        auto st=acc_->processEvent(ev);
        if (st==StreamAccumulator::Done)
        {
            complete();
            return;
        }
        else if (st==StreamAccumulator::Error)
        {
            if (!acc_->receivedContent()
                && isRetryableStreamError(acc_->errorType())
                && attempt_<maxRetries_ )
            {
                scheduleRetry(acc_->errorMessage(), std::min(30, 2<<attempt_));
            }
            else
            {
                fail(acc_->errorMessage());
            }
            return;
        }
    }
}


void StreamingHttpBackend::onReplyFinished(HttpStream::Error netError, const QString& netErrorString)
{
    if (!reply_ || !busy_ || retryTimer_->isActive())
        return; // finished or retry scheduled

    idleTimer_->stop();

    QByteArray retryAfter=retryAfter_;

    if (httpStatus_==200)
    {
        for (const auto& ev: sse_.finish())
            acc_->processEvent(ev);
        auto st=acc_->finishStream();
        if (st==StreamAccumulator::Done)
            complete();
        else if (st==StreamAccumulator::Error)
            fail(acc_->errorMessage());
        else
            fail(netErrorString.isEmpty()
                     ? QString("The response stream ended unexpectedly")
                     : QString("The response stream ended unexpectedly (%1)").arg(netErrorString));
        return;
    }

    bool retry=false;
    QString msg;
    if (httpStatus_==0)
    {
        // no HTTP response at all
        msg=QString("Network error: %1").arg(netErrorString);
        retry = netError!=HttpStream::Canceled
                && netError!=HttpStream::ConnectionRefused
                && netError!=HttpStream::HostNotFound;
    }
    else
    {
        msg=errorMessageFromBody(httpStatus_, errorBody_);
        retry = (httpStatus_==429 || httpStatus_==500 || httpStatus_==502
                 || httpStatus_==503 || httpStatus_==504 || httpStatus_==529);
    }

    if (httpStatus_!=0 && attempt_<maxRetries_
        && alternativeRequest(httpStatus_, errorBody_, requestBody_))
    {
        alternativeRequestUsed_=true;
        scheduleRetry(msg+" - trying an alternative request", 0);
        return;
    }

    if (retry && attempt_<maxRetries_)
    {
        bool ok=false;
        int delay=retryAfter.toInt(&ok);
        if (!ok || delay<=0)
            delay=std::min(30, 2<<attempt_);
        scheduleRetry(msg, std::min(delay, 120));
    }
    else
    {
        fail(msg);
    }
}


void StreamingHttpBackend::onIdleTimeout()
{
    if (!busy_)
        return;
    fail(QString("No response within %1 s").arg(idleTimeoutSeconds_));
}


void StreamingHttpBackend::fail(const QString& msg)
{
    abortReply();
    retryTimer_->stop();
    if (busy_)
    {
        busy_=false;
        Q_EMIT turnFailed(msg);
    }
}


void StreamingHttpBackend::failLater(const QString& msg)
{
    cancel();
    busy_=true;
    QTimer::singleShot(0, this, [this, msg]() { fail(msg); });
}


void StreamingHttpBackend::abortReply()
{
    if (reply_)
    {
        HttpStream* r=reply_;
        reply_=nullptr;
        r->disconnect(this);
        r->abort();
        r->deleteLater();
    }
    idleTimer_->stop();
}


void StreamingHttpBackend::cancel()
{
    busy_=false;
    retryTimer_->stop();
    abortReply();
}


bool StreamingHttpBackend::isBusy() const
{
    return busy_;
}


} // namespace assistant
} // namespace insight
