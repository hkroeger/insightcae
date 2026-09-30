/*
 * Background processes on the remote server
 * (used for the "analyze --server" process of a remote run).
 */

#include "base/linuxremoteserver.h"
#include "base/remoteexecution.h"

#include "backends.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_server_backgroundjob", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;

    {
        BackendSession session(be);

        // unique marker for the processes of this test run
        std::string marker = std::to_string(3000 + (getpid()%5000));


        tr.run("background process is started and killed", [&]()
        {
            auto srv = be->server();
            std::string cmd = "sleep "+marker+"1";

            auto job = srv->launchBackgroundProcess(cmd);
            check(bool(job), "no job returned");

            check(waitFor([&]{ return be->remoteProcessesMatching(cmd).size()==1; }, 10s),
                  "background process not found on the remote side");

            job->kill();

            check(waitFor([&]{ return be->remoteProcessesMatching(cmd).empty(); }, 10s),
                  "background process still running after kill()");
        });


        tr.run("background process with redirected output keeps running after launch", [&]()
        {
            auto srv = be->server();
            auto d = be->scratchDirectory();
            std::string cmd =
                "bash -c 'for i in $(seq 1 30); do echo tick $i; sleep 1; done' >'"
                +toUnixPath(d/"out.log")+"' 2>&1 </dev/null";

            auto job = srv->launchBackgroundProcess(cmd);

            check(waitFor([&]
                {
                    int ret;
                    auto out = be->remoteOutput("cat '"+toUnixPath(d/"out.log")+"'", &ret);
                    return out.find("tick 3")!=std::string::npos;
                }, 20s),
                "background process did not continue to produce output" );

            job->kill();
        });


        tr.run("kill() of an already finished process is harmless", [&]()
        {
            auto srv = be->server();
            auto job = srv->launchBackgroundProcess("true");
            std::this_thread::sleep_for(1s);
            job->kill();
            job->kill();
        });


        tr.run("expected output is captured before detaching", [&]()
        {
            auto srv = be->server();
            std::vector<std::string> m;
            auto job = srv->launchBackgroundProcess(
                "echo READY===4242===READY; sleep "+marker+"3",
                { { boost::regex("READY===([0-9]+)===READY"), &m } } );
            check(m.size()==2 && m[1]=="4242", "expected output not captured");
            job->kill();
        });


        tr.run("killing a job kills the whole process tree", [&]()
        {
            // "analyze" runs solvers as child processes: they must not survive
            auto srv = be->server();
            std::string child = "sleep "+marker+"4";
            auto job = srv->launchBackgroundProcess(
                "bash -c '"+child+"; echo done'" );

            check(waitFor([&]{ return be->remoteProcessesMatching(child).size()==1; }, 10s),
                  "child process not started");

            job->kill();

            bool gone = waitFor([&]{ return be->remoteProcessesMatching(child).empty(); }, 10s);
            if (!gone)
                be->remoteOutput("pkill -f '"+child+"'");
            check(gone, "child process survived kill() of the background job");
        });


        tr.run("analyze server launched through RemoteExecutionConfig is killed", [&]()
        {
            // (formerly test/unit/toolkit/toolkit_remoteexecutionconfig.cpp)
            TemporaryDirectory localDir;
            auto rd = be->scratchDirectory();

            RemoteExecutionConfig rec(be->serverConfig(), localDir.path(), rd);
            check(rec.remoteDirExists(), "remote directory not found");

            {
                auto rs = rec.server()->remoteOFStream(rd/"param.ist", 0);
                writeDummyAnalysisInputFile(rs->stream(), 600, 500, false);
            }

            int port = be->server()->findFreeRemotePort();
            std::string pattern = "analyze.*--port "+std::to_string(port);
            auto process = rec.server()->launchBackgroundProcess(
                "analyze --workdir=\""+toUnixPath(rd)+"\" --server"
                " --port "+std::to_string(port)+
                " \""+toUnixPath(rd/"param.ist")+"\""
                " >\""+toUnixPath(rd/"analyze.log")+"\" 2>&1 </dev/null" );

            check(waitFor([&]{ return be->remoteProcessesMatching(pattern).size()>0; }, 10s),
                  "analyze server process not found");

            process->kill();

            check(waitFor([&]{ return be->remoteProcessesMatching(pattern).empty(); }, 10s),
                  "analyze server still running after kill()");
        });


        tr.run("launch failure is reported instead of hanging", [&]()
        {
            // the remote shell exits before the PID is reported
            // (like a failing ssh connection)
            auto cfg = be->serverConfig();
            checkCompletesWithin(10s, [cfg]()
            {
                auto srv = cfg->instance();
                expectThrows<insight::Exception>(
                    [&](){ srv->launchBackgroundProcess("exit 1; true"); },
                    "launching a background process, which does not report its PID" );
            }, "launchBackgroundProcess with failing remote shell");
        });
    }

    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
