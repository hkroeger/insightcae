#include "httpstream.h"

#include <atomic>
#include <mutex>
#include <thread>

#include <curl/curl.h>

#include <QMetaObject>
#include <QPointer>

namespace insight {
namespace assistant {


struct HttpStream::State
{
    std::atomic<bool> aborted{false};

    std::mutex mutex;
    HttpStream* target = nullptr; // cleared by the destructor of the HttpStream

    // response header (worker thread only)
    int status = 0;
    QByteArray retryAfter;
    bool headerReported = false;

    // call f in the thread of the target object, if it still exists
    template<class F>
    void post(F&& f)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (target && !aborted)
        {
            QPointer<HttpStream> t(target);
            QMetaObject::invokeMethod(
                target,
                [t, f]() { if (t) f(t.data()); },
                Qt::QueuedConnection );
        }
    }

    void reportHeader()
    {
        if (!headerReported)
        {
            headerReported=true;
            int s=status;
            QByteArray ra=retryAfter;
            post([s, ra](HttpStream* h) { Q_EMIT h->responseStarted(s, ra); });
        }
    }
};




namespace {

std::once_flag curlInit;


size_t headerCallback(char* buffer, size_t size, size_t n, void* userdata)
{
    auto* st=static_cast<HttpStream::State*>(userdata);
    QByteArray line=QByteArray(buffer, int(size*n)).trimmed();

    if (line.startsWith("HTTP/"))
    {
        // a new response (after 100-continue or a redirect): the last one counts
        auto parts=line.split(' ');
        st->status = parts.size()>1 ? parts[1].toInt() : 0;
        st->retryAfter.clear();
    }
    else
    {
        int c=line.indexOf(':');
        if (c>0 && line.left(c).trimmed().toLower()=="retry-after")
            st->retryAfter=line.mid(c+1).trimmed();
    }
    return st->aborted ? 0 : size*n;
}


size_t writeCallback(char* buffer, size_t size, size_t n, void* userdata)
{
    auto* st=static_cast<HttpStream::State*>(userdata);
    if (st->aborted)
        return 0;
    st->reportHeader();
    QByteArray data(buffer, int(size*n));
    st->post([data](HttpStream* h) { Q_EMIT h->dataReceived(data); });
    return size*n;
}


int progressCallback(void* userdata, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
{
    auto* st=static_cast<HttpStream::State*>(userdata);
    return st->aborted ? 1 : 0;
}


HttpStream::Error mapError(CURLcode c)
{
    switch (c)
    {
    case CURLE_OK:
        return HttpStream::NoError;
    case CURLE_ABORTED_BY_CALLBACK:
    case CURLE_WRITE_ERROR:
        return HttpStream::Canceled;
    case CURLE_COULDNT_CONNECT:
        return HttpStream::ConnectionRefused;
    case CURLE_COULDNT_RESOLVE_HOST:
    case CURLE_COULDNT_RESOLVE_PROXY:
        return HttpStream::HostNotFound;
    case CURLE_OPERATION_TIMEDOUT:
        return HttpStream::Timeout;
    default:
        return HttpStream::OtherError;
    }
}


void perform(
    std::shared_ptr<HttpStream::State> st,
    QByteArray method,
    QByteArray url,
    QList<QByteArray> headers,
    QByteArray body )
{
    CURL* curl=curl_easy_init();
    if (!curl)
    {
        st->post([](HttpStream* h) { Q_EMIT h->finished(HttpStream::OtherError, "could not initialize libcurl"); });
        return;
    }

    char errbuf[CURL_ERROR_SIZE];
    errbuf[0]=0;

    curl_slist* hl=nullptr;
    for (const auto& h: headers)
        hl=curl_slist_append(hl, h.constData());
    hl=curl_slist_append(hl, "Expect:");

    curl_easy_setopt(curl, CURLOPT_URL, url.constData());
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hl);
    curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, &headerCallback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, st.get());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, st.get());
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, &progressCallback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, st.get());
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    if (method=="POST")
    {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.constData());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE, curl_off_t(body.size()));
    }
    else if (method!="GET")
    {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.constData());
    }

    CURLcode res=curl_easy_perform(curl);

    if (res==CURLE_OK)
        st->reportHeader(); // response without body

    auto err=mapError(res);
    QString msg;
    if (res!=CURLE_OK)
    {
        msg=QString::fromUtf8(curl_easy_strerror(res));
        QString detail=QString::fromUtf8(errbuf).trimmed();
        if (!detail.isEmpty() && detail!=msg)
            msg+=": "+detail;
    }

    curl_slist_free_all(hl);
    curl_easy_cleanup(curl);

    st->post([err, msg](HttpStream* h) { Q_EMIT h->finished(err, msg); });
}

} // anonymous namespace




HttpStream::HttpStream(QObject* parent)
    : QObject(parent),
      state_(std::make_shared<State>())
{
    state_->target=this;
    connect(this, &HttpStream::finished, this, [this]() { finished_=true; });
}


HttpStream::~HttpStream()
{
    abort();
    std::lock_guard<std::mutex> lock(state_->mutex);
    state_->target=nullptr;
}


HttpStream* HttpStream::start(
    const QByteArray& method,
    const QNetworkRequest& req,
    const QByteArray& body,
    QObject* parent )
{
    std::call_once(curlInit, []()
    {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        qRegisterMetaType<HttpStream::Error>();
    });

    auto* h=new HttpStream(parent);

    QList<QByteArray> headers;
    auto ct=req.header(QNetworkRequest::ContentTypeHeader);
    if (ct.isValid())
        headers<<"Content-Type: "+ct.toByteArray();
    for (const auto& n: req.rawHeaderList())
        headers<<n+": "+req.rawHeader(n);

    // the thread keeps the state alive; the HttpStream object may be deleted earlier
    std::thread(
        &perform, h->state_, method,
        req.url().toEncoded(), headers, body ).detach();

    return h;
}


void HttpStream::abort()
{
    state_->aborted=true;
}


} // namespace assistant
} // namespace insight
