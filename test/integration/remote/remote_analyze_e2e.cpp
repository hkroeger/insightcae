/*
 * End-to-end: the sequence of a remote run (as performed by RemoteRun in the workbench),
 * without the GUI:
 *
 *  1. set up temporary remote location, find free remote port
 *  2. upload input file
 *  3. launch "analyze --server" in the background
 *  4. make the port accessible, contact the server
 *  5. monitor, fetch results
 *  6. stop the server
 *  7. download, clean up
 */

#include <condition_variable>
#include <mutex>

#include "analyzeclient.h"
#include "base/linuxremoteserver.h"
#include "base/remotelocation.h"

#include "backends.h"
#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;




bool queryStatus(AnalyzeClient& ac, QueryStatusAction::Result& result)
{
    struct W { std::mutex mx; std::condition_variable cv; bool done=false; QueryStatusAction::Result r; };
    auto w=std::make_shared<W>();
    ac.queryStatus(
        [w](QueryStatusAction::Result r)
        { std::lock_guard<std::mutex> l(w->mx); w->r=r; w->done=true; w->cv.notify_all(); },
        [w]()
        { std::lock_guard<std::mutex> l(w->mx); w->done=true; w->cv.notify_all(); } );
    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, 20s, [w]{ return w->done; }))
    {
        ac.httpClient().abort();
        ac.forgetRequest();
        return false;
    }
    result=w->r;
    return true;
}


bool pollUntil(
    AnalyzeClient& ac,
    std::function<bool(const QueryStatusAction::Result&)> pred,
    std::chrono::milliseconds timeout,
    QueryStatusAction::Result* last=nullptr )
{
    auto deadline=std::chrono::steady_clock::now()+timeout;
    while (std::chrono::steady_clock::now()<deadline)
    {
        QueryStatusAction::Result r;
        if (queryStatus(ac, r) && r.success)
        {
            if (last) *last=r;
            if (pred(r)) return true;
        }
        std::this_thread::sleep_for(500ms);
    }
    return false;
}


bool control(AnalyzeClient& ac, const std::string& action)
{
    struct W { std::mutex mx; std::condition_variable cv; bool done=false, success=false; };
    auto w=std::make_shared<W>();
    auto cb=[w](AnalyzeClientAction::ReportSuccessResult r)
    { std::lock_guard<std::mutex> l(w->mx); w->success=r.success; w->done=true; w->cv.notify_all(); };
    auto to=[w]()
    { std::lock_guard<std::mutex> l(w->mx); w->done=true; w->cv.notify_all(); };
    if (action=="kill") ac.kill(cb, to); else ac.exit(cb, to);
    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, 20s, [w]{ return w->done; }))
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
        { std::lock_guard<std::mutex> l(w->mx); w->r=std::move(r.results); w->done=true; w->cv.notify_all(); },
        [w]()
        { std::lock_guard<std::mutex> l(w->mx); w->done=true; w->cv.notify_all(); } );
    std::unique_lock<std::mutex> l(w->mx);
    if (!w->cv.wait_for(l, 60s, [w]{ return w->done; }))
        return nullptr;
    return std::move(w->r);
}




/**
 * a remote run in progress: steps 1-4
 */
struct RemoteAnalyzeRun
{
    RemoteBackend& be;
    std::unique_ptr<RemoteLocation> loc;
    int port;
    RemoteServer::BackgroundJobPtr job;
    RemoteServer::PortMappingPtr portMapping;
    std::unique_ptr<AnalyzeClient> client;

    std::string processPattern() const
    {
        return "analyze.*--port "+std::to_string(port);
    }

    RemoteAnalyzeRun(
        RemoteBackend& backend,
        double executionTime, int deltaT, bool emitError,
        std::function<void(int)> beforeLaunch = std::function<void(int)>() )
        : be(backend)
    {
        // 1.
        loc = std::make_unique<RemoteLocation>(be.serverConfig());
        loc->initialize(true);
        port = loc->port();
        auto rd = loc->remoteDir();

        // 2.
        {
            auto rs = loc->server()->remoteOFStream(rd/"param.ist", 0);
            writeDummyAnalysisInputFile(rs->stream(), executionTime, deltaT, emitError);
            rs->close();
        }

        if (beforeLaunch) beforeLaunch(port);

        // 3. (same command line as RemoteRun::launchRemoteExecutionServer)
        job = loc->server()->launchBackgroundProcess(
            "analyze "
            " --workdir=\""+toUnixPath(rd)+"\""
            " --server"
            " --port "+std::to_string(port)+
            " param.ist >\""+toUnixPath(rd/"analyze.log")+"\" 2>&1 </dev/null" );

        // 4.
        portMapping = loc->server()->makePortsAccessible({port}, {});
        client = std::make_unique<AnalyzeClient>(
            "Dummy Analysis",
            "http://"+loc->server()->IPaddress()+":"
                +std::to_string(portMapping->localListenerPort(port)),
            nullptr );
        client->setTimeout(10s);
    }

    ~RemoteAnalyzeRun()
    {
        client.reset();
        portMapping.reset();
        try
        {
            if (!be.remoteProcessesMatching(processPattern()).empty())
                job->kill();
        }
        catch (...) {}
        try
        {
            if (loc && be.remoteDirectoryExists(loc->remoteDir()))
                be.server()->removeDirectory(loc->remoteDir());
        }
        catch (...) {}
    }

    std::string remoteLog()
    {
        int ret;
        auto l = be.remoteOutput("cat '"+toUnixPath(loc->remoteDir()/"analyze.log")+"'", &ret);
        if (l.size()>3000) l="..."+l.substr(l.size()-3000);
        return l;
    }

    void waitForContact()
    {
        // fail fast, if the remote analyze has exited (e.g. invalid input)
        auto deadline=std::chrono::steady_clock::now()+60s;
        while (std::chrono::steady_clock::now()<deadline)
        {
            if (pollUntil(*client, [](const QueryStatusAction::Result&){ return true; }, 2s))
                return;
            check(job->isRunning(),
                  "the remote analyze server has exited before it could be contacted. Log:\n"+remoteLog());
        }
        fail("no contact to remote analyze server within 60 s. Log:\n"+remoteLog());
    }

    QueryStatusAction::Result waitForEnd(std::chrono::milliseconds timeout)
    {
        QueryStatusAction::Result last;
        bool finished = pollUntil(
            *client,
            [](const QueryStatusAction::Result& r){ return r.resultsAreAvailable || r.errorOccurred; },
            timeout, &last );
        check(finished, "remote analysis did not finish. Log:\n"+remoteLog());
        return last;
    }

    bool processGoneWithin(std::chrono::milliseconds t)
    {
        return waitFor([this]{ return be.remoteProcessesMatching(processPattern()).empty(); }, t, 250ms);
    }
};




int main(int argc, char* argv[])
{
    TestRunner tr("remote_analyze_e2e", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;

    {
        BackendSession session(be);


        tr.run("successful remote run", [&]()
        {
            RemoteAnalyzeRun run(*be, 3, 100, false);
            run.waitForContact();

            auto st = run.waitForEnd(180s);
            check(!st.errorOccurred, "analysis failed: "+
                  (st.exception ? st.exception->message() : std::string("?")));

            auto res = fetchResults(*run.client);
            check(bool(res), "results could not be fetched");

            check(control(*run.client, "exit"), "exit request failed");
            check(run.processGoneWithin(30s), "remote analyze still running after exit request");

            TemporaryDirectory localDir;
            run.loc->syncToLocal(localDir.path(), false);
            check(fs::exists(localDir.path()/"param.ist"), "input file not downloaded");
            check(fs::exists(localDir.path()/"analyze.log"), "log file not downloaded");

            auto rd = run.loc->remoteDir();
            run.loc->cleanup();
            check(!be->remoteDirectoryExists(rd), "temporary remote directory not removed by cleanup");
        });


        tr.run("error in remote analysis", [&]()
        {
            RemoteAnalyzeRun run(*be, 2, 100, true);
            run.waitForContact();

            auto st = run.waitForEnd(180s);
            check(st.errorOccurred, "error expected");
            check(st.exception && st.exception->message().find("You wanted this error.")!=std::string::npos,
                  "unexpected error message");

            check(control(*run.client, "exit"), "exit request failed");
            check(run.processGoneWithin(30s), "remote analyze still running after exit request");
        });


        tr.run("cancellation of a running remote analysis", [&]()
        {
            RemoteAnalyzeRun run(*be, 600, 200, false);
            run.waitForContact();
            std::this_thread::sleep_for(2s);

            check(control(*run.client, "kill"), "kill request failed");
            // the interruption has to be reported through the status
            run.waitForEnd(60s);
            check(control(*run.client, "exit"), "exit request failed");
            check(run.processGoneWithin(30s), "remote analyze still running after kill and exit");
        });


        tr.run("killing the background job ends the remote analyze", [&]()
        {
            // rollback path of RemoteRun
            RemoteAnalyzeRun run(*be, 600, 200, false);
            run.waitForContact();

            run.job->kill();
            check(run.processGoneWithin(20s), "remote analyze still running after killing the job");
        });


        tr.run("occupied remote port makes the remote analyze fail", [&]()
        {
            if (!be->isLocalMachine())
                throw SkipCase("port occupation requires a backend on the local machine");

            std::unique_ptr<PortBlocker> blocker;
            RemoteAnalyzeRun run(*be, 600, 200, false,
                [&](int port){ blocker=std::make_unique<PortBlocker>(port, true); } );

            bool gone = run.processGoneWithin(60s);
            check(gone,
                  "remote analyze keeps running although it could not start its REST server. Log:\n"
                  +run.remoteLog());
        });
    }

    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
