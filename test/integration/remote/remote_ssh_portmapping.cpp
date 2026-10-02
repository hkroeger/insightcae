/*
 * SSH port tunnels (SSHLinuxServer::makePortsAccessible),
 * used to reach the REST server of a remote analyze process.
 *
 * The "remote" listener is a local fake server, so this test requires
 * an SSH backend on the local machine (ssh-localhost).
 */

#include <condition_variable>
#include <mutex>

#include "analyzeclient.h"
#include "base/sshlinuxserver.h"

#include "backends.h"
#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;




/**
 * @return true, if a status query succeeded; false on failure. Throws on timeout.
 */
bool statusQuerySucceeds(const std::string& url, std::chrono::milliseconds timeout)
{
    struct W { std::mutex mx; std::condition_variable cv; bool done=false, success=false; };
    auto w=std::make_shared<W>();

    AnalyzeClient ac("test", url, nullptr);
    ac.setTimeout(timeout);
    ac.queryStatus(
        [w](QueryStatusAction::Result r)
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->success=r.success; w->done=true; w->cv.notify_all();
        },
        [w]()
        {
            std::lock_guard<std::mutex> l(w->mx);
            w->done=true; w->cv.notify_all();
        } );

    std::unique_lock<std::mutex> l(w->mx);
    check(w->cv.wait_for(l, timeout+5s, [w]{ return w->done; }),
          "no answer from "+url+" within timeout");
    return w->success;
}




int main(int argc, char* argv[])
{
    TestRunner tr("remote_ssh_portmapping", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;
    if (!be->isSSH() || !be->isLocalMachine())
    {
        std::cout<<"requires an SSH backend on the local machine: skipping"<<std::endl;
        return SKIP_RETURN_CODE;
    }

    {
        BackendSession session(be);


        tr.run("remote listener is reachable through the tunnel", [&]()
        {
            FakeAnalyzeServer remoteListener;
            auto srv = be->server();

            auto pm = srv->makePortsAccessible({remoteListener.port()}, {});
            int localPort = pm->localListenerPort(remoteListener.port());
            check(localPort!=remoteListener.port(), "tunnel should use a different local port");

            std::string url="http://127.0.0.1:"+std::to_string(localPort);
            // the tunnel may need some time to come up
            bool ok = waitFor([&]{ return statusQuerySucceeds(url, 5s); }, 30s, 500ms);
            check(ok, "remote listener not reachable through the tunnel");
            check(remoteListener.requestCount()>0, "no request arrived at the remote listener");
        });


        tr.run("tunnel is usable as soon as it is returned", [&]()
        {
            FakeAnalyzeServer remoteListener;
            auto pm = be->server()->makePortsAccessible({remoteListener.port()}, {});
            std::string url="http://127.0.0.1:"+std::to_string(
                pm->localListenerPort(remoteListener.port()));
            check(statusQuerySucceeds(url, 5s),
                  "first request through a freshly created tunnel failed "
                  "(tunnel is not ready when makePortsAccessible returns)");
        });


        tr.run("request fails quickly, if nothing listens on the remote port", [&]()
        {
            int freePort;
            {
                PortBlocker b; // just to obtain a free port
                freePort=b.port();
            }
            auto pm = be->server()->makePortsAccessible({freePort}, {});
            std::string url="http://127.0.0.1:"+std::to_string(pm->localListenerPort(freePort));
            std::this_thread::sleep_for(2s); // let the tunnel come up

            auto t0=std::chrono::steady_clock::now();
            bool ok = statusQuerySucceeds(url, 10s);
            auto dt=std::chrono::steady_clock::now()-t0;
            check(!ok, "request should fail");
            check(dt<5s, "failure took %d s", int(std::chrono::duration_cast<std::chrono::seconds>(dt).count()));
        });


        tr.run("tunnel process is terminated with the mapping", [&]()
        {
            FakeAnalyzeServer remoteListener;
            int localPort;
            {
                auto pm = be->server()->makePortsAccessible({remoteListener.port()}, {});
                localPort=pm->localListenerPort(remoteListener.port());
                std::string pattern="ssh.*-L "+std::to_string(localPort)+":";
                check(waitFor([&]{ return localProcessesMatching(pattern).size()==1; }, 10s),
                      "tunnel process not found");
            }
            std::string pattern="ssh.*-L "+std::to_string(localPort)+":";
            check(waitFor([&]{ return localProcessesMatching(pattern).empty(); }, 10s),
                  "tunnel process still running after destruction of the mapping");
        });


        tr.run("broken tunnel is detected", [&]()
        {
            FakeAnalyzeServer remoteListener;
            auto pm = be->server()->makePortsAccessible({remoteListener.port()}, {});
            int localPort=pm->localListenerPort(remoteListener.port());
            std::string url="http://127.0.0.1:"+std::to_string(localPort);
            check(waitFor([&]{ return statusQuerySucceeds(url, 5s); }, 30s, 500ms),
                  "tunnel not working");

            auto tpm = std::dynamic_pointer_cast<SSHLinuxServer::SSHTunnelPortMapping>(pm);
            check(bool(tpm), "unexpected port mapping type");
            tpm->tunnelProcess_.terminate();

            auto t0=std::chrono::steady_clock::now();
            bool ok = statusQuerySucceeds(url, 10s);
            auto dt=std::chrono::steady_clock::now()-t0;
            check(!ok, "request through a terminated tunnel should fail");
            check(dt<5s, "failure detection took %d s",
                  int(std::chrono::duration_cast<std::chrono::seconds>(dt).count()));
        });


        tr.run("tunnel to an unreachable host is reported", [&]()
        {
            auto cfg = std::make_shared<SSHLinuxServer::Config>(
                "/tmp", 1, "nonexistent-host.invalid" );

            checkCompletesWithin(30s, [cfg]()
            {
                SSHLinuxServer srv(*cfg);
                expectThrows<insight::Exception>(
                    [&](){ srv.makePortsAccessible({8090}, {}); },
                    "creating a tunnel to an unreachable host" );
            }, "tunnel creation to unreachable host");
        });
    }

    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
