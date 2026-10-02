/*
 * The REST server of "analyze --server" (src/analyze/restapi.cpp),
 * started locally with the "Dummy Analysis" and driven through insight::AnalyzeClient.
 */

#include <condition_variable>
#include <iostream>
#include <mutex>

#include "boost/process.hpp"

#include "analyzeclient.h"
#include "base/remotecommand.h"

#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace bp = boost::process;
namespace fs = boost::filesystem;




class AnalyzeServerProcess
{
public:
    TemporaryDirectory workDir{"remotetest-analyze"};
    int port;
    fs::path logFile;
    std::unique_ptr<bp::child> process;

    AnalyzeServerProcess(
        double executionTime, int deltaT, bool emitError,
        int serverPort=-1 )
    {
        port = serverPort>0 ? serverPort : findFreePort();
        logFile = workDir.path()/"analyze.log";

        writeDummyAnalysisInputFile(
            workDir.path()/"param.ist",
            executionTime, deltaT, emitError );

        process = std::make_unique<bp::child>(
            analyzeExecutable(),
            bp::args({
                "--workdir="+workDir.path().string(),
                "--server",
                "--port", std::to_string(port),
                (workDir.path()/"param.ist").string()
            }),
            bp::start_dir=workDir.path(),
            (bp::std_out & bp::std_err) > logFile,
            bp::std_in < bp::null );
    }

    ~AnalyzeServerProcess()
    {
        if (process)
        {
            std::error_code ec;
            if (process->running(ec))
                process->terminate(ec);
            process->wait(ec);
        }
    }

    std::string url() const
    {
        return "http://127.0.0.1:"+std::to_string(port);
    }

    std::string log() const
    {
        return fs::exists(logFile) ? readFile(logFile) : std::string("(no log)");
    }

    std::string logTail(size_t n=3000) const
    {
        auto l=log();
        if (l.size()>n) l="..."+l.substr(l.size()-n);
        return l;
    }

    bool running()
    {
        std::error_code ec;
        return process->running(ec);
    }

    /**
     * @return true, if the process exited within timeout
     */
    bool waitForExit(std::chrono::milliseconds timeout, int* exitCode=nullptr)
    {
        bool exited = waitFor([this]{ return !running(); }, timeout);
        if (exited)
        {
            std::error_code ec;
            process->wait(ec);
            if (exitCode) *exitCode=process->exit_code();
        }
        return exited;
    }
};




struct StatusWaiter
{
    std::mutex mx;
    std::condition_variable cv;
    bool done=false;
    QueryStatusAction::Result r;
};


/**
 * query status once, synchronously
 */
bool queryStatus(AnalyzeClient& ac, QueryStatusAction::Result& result, std::chrono::milliseconds timeout=10s)
{
    auto w = std::make_shared<StatusWaiter>();
    ac.queryStatus(
        [w](QueryStatusAction::Result r)
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->r=r;
            w->done=true;
            w->cv.notify_all();
        },
        [w]()
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->r.success=false;
            w->done=true;
            w->cv.notify_all();
        } );
    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, timeout, [w]{ return w->done; }))
    {
        ac.httpClient().abort();
        ac.forgetRequest();
        return false;
    }
    result=w->r;
    return true;
}


/**
 * poll the status until pred returns true or timeout
 */
bool pollStatusUntil(
    AnalyzeClient& ac,
    std::function<bool(const QueryStatusAction::Result&)> pred,
    std::chrono::milliseconds timeout,
    QueryStatusAction::Result* last=nullptr )
{
    auto deadline=std::chrono::steady_clock::now()+timeout;
    while (std::chrono::steady_clock::now()<deadline)
    {
        QueryStatusAction::Result r;
        if (queryStatus(ac, r, 10s))
        {
            if (last) *last=r;
            if (r.success && pred(r)) return true;
        }
        std::this_thread::sleep_for(200ms);
    }
    return false;
}


bool control(AnalyzeClient& ac, const std::string& action)
{
    struct W { std::mutex mx; std::condition_variable cv; bool done=false, success=false; };
    auto w=std::make_shared<W>();
    auto cb=[w](AnalyzeClientAction::ReportSuccessResult r)
    {
        std::lock_guard<std::mutex> l(w->mx);
        w->success=r.success; w->done=true; w->cv.notify_all();
    };
    auto to=[w]()
    {
        std::lock_guard<std::mutex> l(w->mx);
        w->done=true; w->cv.notify_all();
    };
    if (action=="kill") ac.kill(cb, to);
    else if (action=="exit") ac.exit(cb, to);
    else if (action=="wnow") ac.wnow(cb, to);
    else if (action=="wnowandstop") ac.wnowandstop(cb, to);
    else fail("unknown action "+action);

    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, 10s, [w]{ return w->done; }))
    {
        ac.httpClient().abort();
        ac.forgetRequest();
        return false;
    }
    return w->success;
}


std::unique_ptr<ResultSet> fetchResults(AnalyzeClient& ac)
{
    struct W { std::mutex mx; std::condition_variable cv; bool done=false; std::unique_ptr<ResultSet> r; };
    auto w=std::make_shared<W>();
    ac.queryResults(
        [w](QueryResultsAction::Result r)
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->r=std::move(r.results); w->done=true; w->cv.notify_all();
        },
        [w]()
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->done=true; w->cv.notify_all();
        } );
    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, 30s, [w]{ return w->done; }))
        return nullptr;
    return std::move(w->r);
}


class CountingProgressDisplayer : public ProgressDisplayer
{
public:
    std::atomic<int> nUpdates{0}, nLogLines{0};
    void setActionProgressValue(const std::string &, double) override {}
    void setMessageText(const std::string &, const std::string&) override {}
    void finishActionProgress(const std::string &) override {}
    void reset() override {}
    void update ( const ProgressState& ) override { nUpdates++; }
    void logMessage(const std::string&) override { nLogLines++; }
};


void waitForContact(AnalyzeClient& ac, AnalyzeServerProcess& p)
{
    bool contact = pollStatusUntil(
        ac, [](const QueryStatusAction::Result&){ return true; }, 60s );
    check(contact, "could not contact analyze server within 60 s. Log:\n"+p.logTail());
}




int main(int argc, char* argv[])
{
    TestRunner tr("remote_analyzeserver_local", argc, argv);


    tr.run("full lifecycle: progress, results, exit", [&]()
    {
        AnalyzeServerProcess p(3, 100, false);
        CountingProgressDisplayer pd;
        AnalyzeClient ac("Dummy Analysis", p.url(), &pd);

        waitForContact(ac, p);

        QueryStatusAction::Result last;
        bool finished = pollStatusUntil(
            ac, [](const QueryStatusAction::Result& r){ return r.resultsAreAvailable || r.errorOccurred; },
            120s, &last );
        check(finished, "analysis did not finish within 120 s. Log:\n"+p.logTail());
        check(!last.errorOccurred, "analysis reported an error: "
              +(last.exception?last.exception->message():std::string("?")));
        check(pd.nUpdates>0 || pd.nLogLines>0, "no progress information was transferred");

        auto res = fetchResults(ac);
        check(bool(res), "results could not be fetched");

        check(control(ac, "exit"), "exit request failed");

        int ec=-1;
        check(p.waitForExit(20s, &ec), "analyze did not exit within 20 s after exit request");
        check(ec==0, "analyze exited with code %d. Log:\n%s", ec, p.logTail().c_str());
    });


    tr.run("error in analysis is reported and server exits on request", [&]()
    {
        AnalyzeServerProcess p(2, 100, true);
        AnalyzeClient ac("Dummy Analysis", p.url(), nullptr);

        waitForContact(ac, p);

        QueryStatusAction::Result last;
        bool finished = pollStatusUntil(
            ac, [](const QueryStatusAction::Result& r){ return r.resultsAreAvailable || r.errorOccurred; },
            120s, &last );
        check(finished, "analysis did not finish within 120 s. Log:\n"+p.logTail());
        check(last.errorOccurred, "error should have been reported");
        check(bool(last.exception), "exception missing");
        check(last.exception->message().find("You wanted this error.")!=std::string::npos,
              "unexpected error message: "+last.exception->message());

        check(control(ac, "exit"), "exit request failed");
        check(p.waitForExit(20s), "analyze did not exit within 20 s after exit request");
    });


    tr.run("kill interrupts a running analysis", [&]()
    {
        AnalyzeServerProcess p(600, 200, false);
        AnalyzeClient ac("Dummy Analysis", p.url(), nullptr);

        waitForContact(ac, p);
        std::this_thread::sleep_for(2s);

        check(control(ac, "kill"), "kill request failed");

        QueryStatusAction::Result last;
        bool stopped = pollStatusUntil(
            ac, [](const QueryStatusAction::Result& r){ return r.resultsAreAvailable || r.errorOccurred; },
            30s, &last );
        check(stopped,
              "the interruption was not reported through the status within 30 s after kill "
              "(analyze process %s). Log:\n%s",
              p.running() ? "still running" : "has exited", p.logTail().c_str());

        check(control(ac, "exit"), "exit request failed");
        check(p.waitForExit(20s), "analyze did not exit within 20 s after exit request");
    });


    tr.run("exit request during a running analysis stops the server", [&]()
    {
        AnalyzeServerProcess p(600, 200, false);
        AnalyzeClient ac("Dummy Analysis", p.url(), nullptr);

        waitForContact(ac, p);
        check(control(ac, "exit"), "exit request failed");
        check(p.waitForExit(30s), "analyze did not exit within 30 s after exit request. Log:\n"+p.logTail());
    });


    tr.run("queries after completion do not crash the server", [&]()
    {
        AnalyzeServerProcess p(1, 100, false);
        AnalyzeClient ac("Dummy Analysis", p.url(), nullptr);

        waitForContact(ac, p);
        bool finished = pollStatusUntil(
            ac, [](const QueryStatusAction::Result& r){ return r.resultsAreAvailable || r.errorOccurred; },
            120s );
        check(finished, "analysis did not finish. Log:\n"+p.logTail());

        // the analysis thread is gone at this point
        {
            struct W { std::mutex mx; std::condition_variable cv; bool done=false; };
            auto w=std::make_shared<W>();
            ac.queryExepath(
                [w](QueryExepathAction::Result) { std::lock_guard<std::mutex> l(w->mx); w->done=true; w->cv.notify_all(); },
                [w]() { std::lock_guard<std::mutex> l(w->mx); w->done=true; w->cv.notify_all(); } );
            std::unique_lock<std::mutex> l(w->mx);
            w->cv.wait_for(l, 10s, [w]{ return w->done; });
        }
        std::this_thread::sleep_for(500ms);
        check(p.running(), "analyze server crashed on GET /exepath after completion. Log:\n"+p.logTail());

        control(ac, "wnow");
        std::this_thread::sleep_for(500ms);
        check(p.running(), "analyze server crashed on wnow after completion. Log:\n"+p.logTail());

        control(ac, "wnowandstop");
        std::this_thread::sleep_for(500ms);
        check(p.running(), "analyze server crashed on wnowandstop after completion. Log:\n"+p.logTail());

        QueryStatusAction::Result r;
        check(queryStatus(ac, r) && r.success, "status query after completion failed");

        control(ac, "exit");
        p.waitForExit(20s);
    });


    tr.run("occupied server port makes analyze fail", [&]()
    {
        PortBlocker blocker(0, true);
        check(!blocker.otherSocketCanBind(),
              "test setup: port %d is not occupied exclusively "
              "(another socket with SO_REUSEADDR can bind to it)", blocker.port());

        AnalyzeServerProcess p(600, 200, false, blocker.port());

        int ec=0;
        bool exited = p.waitForExit(30s, &ec);
        check(exited,
              "analyze keeps running although its REST server could not be started "
              "(port %d in use). Log:\n%s", blocker.port(), p.logTail().c_str());
        check(ec!=0, "analyze should exit with an error code, got %d", ec);
    });


    tr.run("rapid polling during a running analysis", [&]()
    {
        AnalyzeServerProcess p(4, 10, false);
        CountingProgressDisplayer pd;
        AnalyzeClient ac("Dummy Analysis", p.url(), &pd);

        waitForContact(ac, p);

        // hammer the server with status requests, while the analysis produces output
        bool finished=false;
        auto deadline=std::chrono::steady_clock::now()+120s;
        while (!finished && std::chrono::steady_clock::now()<deadline)
        {
            QueryStatusAction::Result r;
            if (queryStatus(ac, r, 10s) && r.success)
                finished = r.resultsAreAvailable || r.errorOccurred;
            check(p.running(), "analyze server died during rapid polling. Log:\n"+p.logTail());
        }
        check(finished, "analysis did not finish. Log:\n"+p.logTail());

        control(ac, "exit");
        check(p.waitForExit(20s), "analyze did not exit");
    });


    return tr.finish();
}
