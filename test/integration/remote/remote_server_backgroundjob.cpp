/*
 * Background processes on the remote server
 * (used for the "analyze --server" process of a remote run).
 */

#include "base/linuxremoteserver.h"
#include "base/remoteexecution.h"
#include "base/sshlinuxserver.h"

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

        // unique process names for this test run (argv[0] set by "exec -a"),
        // leftovers end by themselves after 10 minutes
        std::string marker = "insight-remotejob-"+std::to_string(getpid())+"-";


        tr.run("background process is started and killed", [&]()
        {
            auto srv = be->server();
            std::string name = marker+"1";
            std::string pattern = "^"+name+" "; // argv[0] only, not the job's shell

            auto job = srv->launchBackgroundProcess("exec -a "+name+" sleep 600");
            check(bool(job), "no job returned");

            check(waitFor([&]{ return be->remoteProcessesMatching(pattern).size()==1; }, 10s),
                  "background process not found on the remote side");

            job->kill();

            check(waitFor([&]{ return be->remoteProcessesMatching(pattern).empty(); }, 10s),
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
                "echo READY===4242===READY; exec -a "+marker+"3 sleep 600",
                { { boost::regex("READY===([0-9]+)===READY"), &m } } );
            check(m.size()==2 && m[1]=="4242", "expected output not captured");
            job->kill();
        });


        tr.run("killing a job kills the whole process tree", [&]()
        {
            // "analyze" runs solvers as child processes: they must not survive
            auto srv = be->server();
            std::string child = marker+"4";
            std::string pattern = "^"+child+" "; // argv[0] only, not the job's shell
            // subshell: the job's shell remains the parent of the child process
            auto job = srv->launchBackgroundProcess(
                "(exec -a "+child+" sleep 600); echo done" );

            check(waitFor([&]{ return be->remoteProcessesMatching(pattern).size()==1; }, 10s),
                  "child process not started");

            job->kill();

            bool gone = waitFor([&]{ return be->remoteProcessesMatching(pattern).empty(); }, 10s);
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
                rs->close();
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
            // the connection to the server fails
            // (end of output without PID is covered by remote_lookforpattern)
            if (!be->isSSH())
                throw SkipCase("requires an SSH backend");

            auto cfg = std::make_shared<SSHLinuxServer::Config>(
                "/tmp", 1, "nonexistent-host.invalid" );
            checkCompletesWithin(30s, [cfg]()
            {
                auto srv = cfg->instance();
                expectThrows<insight::Exception>(
                    [&](){ srv->launchBackgroundProcess("sleep 1"); },
                    "launching a background process on an unreachable host" );
            }, "launchBackgroundProcess on unreachable host");
        });


        // remove leftovers of failed cases
        // ("[i]nsight" does not match the command line of the shell running pkill)
        be->remoteOutput("pkill -f -- '[i]"+marker.substr(1)+"'");
    }

    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
