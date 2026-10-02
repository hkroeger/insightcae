#include "fakeanalyzeserver.h"

#include <iostream>
#include <sstream>

#include <boost/algorithm/string.hpp>


using boost::asio::ip::tcp;


namespace insight {
namespace remotetest {




static std::string jsonEscape(const std::string& s)
{
    std::ostringstream os;
    for (unsigned char c: s)
    {
        switch (c)
        {
        case '"': os<<"\\\""; break;
        case '\\': os<<"\\\\"; break;
        case '\n': os<<"\\n"; break;
        case '\r': os<<"\\r"; break;
        case '\t': os<<"\\t"; break;
        default:
            if (c<0x20)
            {
                char buf[8];
                snprintf(buf, sizeof(buf), "\\u%04x", c);
                os<<buf;
            }
            else
                os<<c;
        }
    }
    return os.str();
}




std::string FakeAnalyzeServer::statusJson(const Status& s)
{
    std::ostringstream os;
    os<<"{\"states\":[";
    for (size_t i=0; i<s.states.size(); ++i)
    {
        const auto& st=s.states[i];
        if (i) os<<",";
        os<<"{\"time\":"<<st.time<<",\"ProgressVariableList\":{";
        for (size_t j=0; j<st.pvl.size(); ++j)
        {
            if (j) os<<",";
            os<<"\""<<jsonEscape(st.pvl[j].first)<<"\":"<<st.pvl[j].second;
        }
        os<<"},\"logMessage\":\""<<jsonEscape(st.logMessage)<<"\"}";
    }
    os<<"],\"logLines\":[";
    for (size_t i=0; i<s.logLines.size(); ++i)
    {
        if (i) os<<",";
        os<<"\""<<jsonEscape(s.logLines[i])<<"\"";
    }
    os<<"],\"progressStates\":[";
    for (size_t i=0; i<s.progressStates.size(); ++i)
    {
        const auto& ps=s.progressStates[i];
        if (i) os<<",";
        os<<"{\"path\":\""<<jsonEscape(ps.path)<<"\"";
        if (ps.hasValue) os<<",\"double\":"<<ps.value;
        if (ps.hasText) os<<",\"text\":\""<<jsonEscape(ps.text)<<"\"";
        os<<"}";
    }
    os<<"],\"resultsAvailable\":"<<(s.resultsAvailable?"true":"false")
      <<",\"errorOccurred\":"<<(s.errorOccurred?"true":"false")
      <<",\"errorMessage\":\""<<jsonEscape(s.errorMessage)<<"\""
      <<",\"errorStackTrace\":\""<<jsonEscape(s.errorStackTrace)<<"\""
      <<"}";
    return os.str();
}




FakeAnalyzeServer::FakeAnalyzeServer(int port)
    : acceptor_(ios_)
{
    tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), port);
    acceptor_.open(ep.protocol());
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
    acceptor_.bind(ep);
    acceptor_.listen();
    port_ = acceptor_.local_endpoint().port();

    acceptThread_ = std::thread(&FakeAnalyzeServer::acceptLoop, this);
}




FakeAnalyzeServer::~FakeAnalyzeServer()
{
    stopping_=true;

    boost::system::error_code ec;
    {
        // wake up the blocking accept() by connecting
        tcp::socket s(ios_);
        s.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port_), ec);
    }
    if (acceptThread_.joinable()) acceptThread_.join();
    acceptor_.close(ec);

    std::vector<std::thread> threads;
    {
        std::lock_guard<std::mutex> l(mx_);
        for (auto& s: heldSockets_)
        {
            s->shutdown(tcp::socket::shutdown_both, ec);
            s->close(ec);
        }
        heldSockets_.clear();
        threads.swap(connectionThreads_);
    }
    for (auto& t: threads)
        if (t.joinable()) t.join();
}




int FakeAnalyzeServer::port() const
{
    return port_;
}




std::string FakeAnalyzeServer::url() const
{
    return "http://127.0.0.1:"+std::to_string(port_);
}




void FakeAnalyzeServer::setFault(Fault f, int delayMs)
{
    std::lock_guard<std::mutex> l(mx_);
    fault_=f;
    faultDelayMs_=delayMs;
}




void FakeAnalyzeServer::setStatus(const Status& s)
{
    std::lock_guard<std::mutex> l(mx_);
    status_=s;
}




void FakeAnalyzeServer::setResults(const std::string& xml)
{
    std::lock_guard<std::mutex> l(mx_);
    resultsXml_=xml;
}




void FakeAnalyzeServer::setHandler(std::function<Response(const Request&)> handler)
{
    std::lock_guard<std::mutex> l(mx_);
    handler_=handler;
}




std::vector<FakeAnalyzeServer::Request> FakeAnalyzeServer::requests() const
{
    std::lock_guard<std::mutex> l(mx_);
    return requests_;
}




size_t FakeAnalyzeServer::requestCount() const
{
    std::lock_guard<std::mutex> l(mx_);
    return requests_.size();
}




void FakeAnalyzeServer::acceptLoop()
{
    while (!stopping_)
    {
        auto sock = std::make_shared<tcp::socket>(ios_);
        boost::system::error_code ec;
        acceptor_.accept(*sock, ec);
        if (ec || stopping_) break;

        std::lock_guard<std::mutex> l(mx_);
        connectionThreads_.emplace_back(&FakeAnalyzeServer::serve, this, sock);
    }
}




FakeAnalyzeServer::Response FakeAnalyzeServer::defaultResponse(const Request& r) const
{
    Response resp;
    if (r.method=="GET")
    {
        if (r.path=="/next" || r.path=="/all" || r.path=="/latest")
        {
            resp.contentType="application/json";
            resp.body=statusJson(status_);
        }
        else if (r.path=="/results" && !resultsXml_.empty())
        {
            resp.contentType="application/xml";
            resp.body=resultsXml_;
        }
        else if (r.path=="/exepath")
        {
            resp.contentType="text/plain";
            resp.body="/tmp/fake-exepath";
        }
        else
        {
            resp.status=400;
            resp.contentType="text/plain";
            resp.body="Malformed request\n";
        }
    }
    else if (r.method=="POST" && r.contentType=="application/json")
    {
        resp.contentType="text/plain";
        resp.body="OK\n";
    }
    else
    {
        resp.status=400;
        resp.contentType="text/plain";
        resp.body="Malformed request\n";
    }
    return resp;
}




void FakeAnalyzeServer::serve(std::shared_ptr<tcp::socket> sock)
{
    boost::system::error_code ec;

    Fault fault;
    int delayMs;
    {
        std::lock_guard<std::mutex> l(mx_);
        fault=fault_;
        delayMs=faultDelayMs_;
    }

    if (fault==Fault::CloseConnection)
    {
        sock->shutdown(tcp::socket::shutdown_both, ec);
        sock->close(ec);
        return;
    }

    // read header
    boost::asio::streambuf buf;
    boost::asio::read_until(*sock, buf, "\r\n\r\n", ec);
    if (ec) return;

    std::istream is(&buf);
    Request req;
    std::string line;
    std::getline(is, line);
    {
        std::istringstream rl(line);
        std::string version;
        rl>>req.method>>req.path>>version;
    }
    size_t contentLength=0;
    while (std::getline(is, line) && line!="\r")
    {
        auto c = line.find(':');
        if (c==std::string::npos) continue;
        auto key = boost::algorithm::to_lower_copy(line.substr(0, c));
        auto val = boost::algorithm::trim_copy(line.substr(c+1));
        if (key=="content-length") contentLength=std::stoul(val);
        else if (key=="content-type") req.contentType=val;
    }

    // body: rest of buffer + remaining bytes
    std::string body(
        (std::istreambuf_iterator<char>(is)),
        std::istreambuf_iterator<char>() );
    if (body.size()<contentLength)
    {
        std::vector<char> rest(contentLength-body.size());
        boost::asio::read(*sock, boost::asio::buffer(rest), ec);
        body.append(rest.begin(), rest.end());
    }
    req.body=body;

    std::function<Response(const Request&)> handler;
    Response resp;
    {
        std::lock_guard<std::mutex> l(mx_);
        requests_.push_back(req);
        handler=handler_;
        if (!handler) resp=defaultResponse(req);
    }
    if (handler) resp=handler(req);

    switch (fault)
    {
    case Fault::NeverRespond:
    {
        std::lock_guard<std::mutex> l(mx_);
        heldSockets_.push_back(sock);
        return;
    }
    case Fault::DelayResponse:
        for (int t=0; t<delayMs && !stopping_; t+=10)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        break;
    case Fault::NoContentType:
        resp.contentType.clear();
        break;
    case Fault::MalformedJson:
        resp.contentType="application/json";
        resp.body="{\"states\": [ this is not json";
        break;
    case Fault::HttpError:
        resp.status=500;
        resp.contentType="text/plain";
        resp.body="Internal Server Error\n";
        break;
    default:
        break;
    }

    std::ostringstream os;
    os<<"HTTP/1.1 "<<resp.status<<" "<<(resp.status==200?"OK":"Error")<<"\r\n";
    if (!resp.contentType.empty())
        os<<"Content-Type: "<<resp.contentType<<"\r\n";
    os<<"Content-Length: "<<resp.body.size()<<"\r\n";
    os<<"Connection: close\r\n\r\n";
    os<<resp.body;
    auto out=os.str();
    boost::asio::write(*sock, boost::asio::buffer(out), ec);
    sock->shutdown(tcp::socket::shutdown_both, ec);
    sock->close(ec);
}




bool PortBlocker::otherSocketCanBind() const
{
    boost::asio::io_service ios;
    tcp::acceptor other(ios);
    tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), port_);
    boost::system::error_code ec;
    other.open(ep.protocol(), ec);
    if (!ec) other.set_option(tcp::acceptor::reuse_address(true), ec);
    if (!ec) other.bind(ep, ec);
    return !ec;
}




PortBlocker::PortBlocker(int port, bool closeConnections)
    : acceptor_(ios_),
      closeConnections_(closeConnections)
{
    tcp::endpoint ep(boost::asio::ip::address::from_string("127.0.0.1"), port);
    acceptor_.open(ep.protocol());
#ifdef WIN32
    // On Windows, SO_REUSEADDR allows other sockets to bind to the same port
    // (Wt's server sets it): the port has to be occupied exclusively
    acceptor_.set_option(
        boost::asio::detail::socket_option::boolean<SOL_SOCKET, SO_EXCLUSIVEADDRUSE>(true) );
#else
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_.bind(ep);
    acceptor_.listen();
    port_ = acceptor_.local_endpoint().port();

    thread_ = std::thread(
        [this]()
        {
            while (!stopping_)
            {
                auto sock = std::make_shared<tcp::socket>(ios_);
                boost::system::error_code ec;
                acceptor_.accept(*sock, ec);
                if (ec || stopping_) break;
                if (closeConnections_)
                {
                    sock->close(ec);
                }
                else
                {
                    held_.push_back(sock);
                }
            }
        });
}




PortBlocker::~PortBlocker()
{
    stopping_=true;
    boost::system::error_code ec;
    {
        // wake up the blocking accept() by connecting
        tcp::socket s(ios_);
        s.connect(tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), port_), ec);
    }
    if (thread_.joinable()) thread_.join();
    acceptor_.close(ec);
    for (auto& s: held_) s->close(ec);
}




} // namespace remotetest
} // namespace insight
