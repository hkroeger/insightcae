#ifndef INSIGHT_FAKEANALYZESERVER_H
#define INSIGHT_FAKEANALYZESERVER_H

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio.hpp>


namespace insight {
namespace remotetest {


/**
 * @brief The FakeAnalyzeServer class
 * Minimal HTTP/1.1 server, which mimics the REST API of "analyze --server"
 * (see src/analyze/restapi.cpp) and allows to inject faults.
 *
 * Implemented on plain boost::asio sockets (not Wt), so that
 * every byte of the response can be controlled
 * and transport faults (closed connection, no response) can be simulated.
 * Each connection is served by its own thread, one request per connection.
 */
class FakeAnalyzeServer
{
public:
    struct Request
    {
        std::string method, path, contentType, body;
    };

    struct Response
    {
        int status = 200;
        std::string contentType = "application/json"; // omitted, if empty
        std::string body;
    };

    enum class Fault
    {
        None,
        CloseConnection, // accept and close without response
        NeverRespond,    // accept, read request, never answer (until server is destroyed)
        DelayResponse,   // answer after faultDelay
        NoContentType,   // answer without Content-Type header
        MalformedJson,   // answer with invalid JSON body
        HttpError        // answer with status 500
    };

    struct Status
    {
        bool resultsAvailable = false;
        bool errorOccurred = false;
        std::string errorMessage;
        std::string errorStackTrace;
        std::vector<std::string> logLines;
        // (time, {name: value}, logMessage)
        struct State { double time; std::vector<std::pair<std::string,double> > pvl; std::string logMessage; };
        std::vector<State> states;
        // (path, value or text or finish)
        struct ProgressState { std::string path; bool hasValue=false; double value=0; bool hasText=false; std::string text; };
        std::vector<ProgressState> progressStates;
    };

private:
    boost::asio::io_service ios_;
    boost::asio::ip::tcp::acceptor acceptor_;
    int port_;
    std::thread acceptThread_;
    std::atomic<bool> stopping_{false};

    mutable std::mutex mx_;
    std::vector<Request> requests_;
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket> > heldSockets_;
    std::vector<std::thread> connectionThreads_;

    Fault fault_ = Fault::None;
    int faultDelayMs_ = 0;
    Status status_;
    std::string resultsXml_;
    std::function<Response(const Request&)> handler_;

    void acceptLoop();
    void serve(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    Response defaultResponse(const Request& r) const;

public:
    /**
     * @param port
     * port to listen on (127.0.0.1). 0: choose any free port
     */
    FakeAnalyzeServer(int port=0);
    ~FakeAnalyzeServer();

    int port() const;
    std::string url() const;

    void setFault(Fault f, int delayMs=0);
    void setStatus(const Status& s);
    void setResults(const std::string& xml);

    /**
     * @brief setHandler
     * replace the default request handler completely
     */
    void setHandler(std::function<Response(const Request&)> handler);

    std::vector<Request> requests() const;
    size_t requestCount() const;

    static std::string statusJson(const Status& s);
};


/**
 * @brief The PortBlocker class
 * listens on a port without ever answering, or closes each connection right away
 */
class PortBlocker
{
    boost::asio::io_service ios_;
    boost::asio::ip::tcp::acceptor acceptor_;
    int port_;
    bool closeConnections_;
    std::thread thread_;
    std::atomic<bool> stopping_{false};
    std::vector<std::shared_ptr<boost::asio::ip::tcp::socket> > held_;
public:
    PortBlocker(int port=0, bool closeConnections=true);
    ~PortBlocker();
    int port() const { return port_; }
};


} // namespace remotetest
} // namespace insight

#endif // INSIGHT_FAKEANALYZESERVER_H
