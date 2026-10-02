/*
 * Dynamically created / destroyed SSH servers
 * (creationCommand / destructionCommand of SSHLinuxServer::Config).
 *
 * This is the mechanism, which will be extended for cloud instances.
 * The creation and destruction commands are local marker scripts here.
 */

#include <iostream>

#include "base/sshlinuxserver.h"

#include "backends.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_ssh_lifecycle", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;
    if (!be->isSSH())
    {
        std::cout<<"requires an SSH backend: skipping"<<std::endl;
        return SKIP_RETURN_CODE;
    }

    auto sshCfg = std::dynamic_pointer_cast<SSHLinuxServer::Config>(be->serverConfig());
    if (!sshCfg)
    {
        std::cerr<<"backend "<<be->name()<<" has no SSH configuration"<<std::endl;
        return 1;
    }


    tr.run("creation command is executed for a server, which is not running", [&]()
    {
        TemporaryDirectory d;
        auto created = d.path()/"created";

        // the host is unreachable, so the creation command has to be run.
        // Only its execution is checked here (the host stays unreachable).
        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "touch '"+created.string()+"'",
            "true" );
        cfg.creationTimeout_ = 0s; // don't wait for the host

        try { cfg.instance(); } catch (...) {}
        check(fs::exists(created), "creation command was not executed");
    });


    tr.run("server, which is still unreachable after creation, is reported", [&]()
    {
        TemporaryDirectory d;
        auto destroyed = d.path()/"destroyed";

        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "true",   // "creation" succeeds, but host stays unreachable
            "touch '"+destroyed.string()+"'" );
        cfg.creationTimeout_ = 10s;

        checkCompletesWithin(120s, [cfg]()
        {
            expectThrows<insight::Exception>(
                [&](){ cfg.instance(); },
                "instance() of a server, which is not reachable after creation" );
        }, "instance() of unreachable server");

        check(fs::exists(destroyed),
              "the unreachable server was not destroyed after the creation timeout");
    });


    tr.run("failing creation command is reported", [&]()
    {
        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "exit 1",
            "true" );

        expectThrows<insight::Exception>(
            [&](){ cfg.instance(); },
            "instance() with failing creation command" );
    });


    tr.run("creation command is not executed for a running server", [&]()
    {
        TemporaryDirectory d;
        auto created = d.path()/"created";

        SSHLinuxServer::Config cfg(
            sshCfg->defaultDirectory_, 1, sshCfg->hostName_,
            "touch '"+created.string()+"'",
            "true" );

        auto srv = cfg.instance();
        check(bool(srv), "no instance");
        check(!fs::exists(created), "creation command executed although server is running");
    });


    tr.run("destruction command is executed", [&]()
    {
        TemporaryDirectory d;
        auto destroyed = d.path()/"destroyed";

        SSHLinuxServer::Config cfg(
            sshCfg->defaultDirectory_, 1, sshCfg->hostName_,
            "true",
            "touch '"+destroyed.string()+"'" );

        auto srv = cfg.instance();
        srv->destroyIfPossible();
        check(fs::exists(destroyed), "destruction command was not executed");
    });


    tr.run("failing destruction command does not throw", [&]()
    {
        SSHLinuxServer::Config cfg(
            sshCfg->defaultDirectory_, 1, sshCfg->hostName_,
            "true",
            "exit 1" );

        auto srv = cfg.instance();
        srv->destroyIfPossible();
    });


    tr.run("pool expansion inserts the instance id into the commands", [&]()
    {
        SSHLinuxServer::Config cfg(
            "/tmp", 2, "node%d.example",
            "create-node %d",
            "destroy-node %d" );

        check(cfg.isExpandable(), "configuration should be expandable");
        auto e = std::dynamic_pointer_cast<SSHLinuxServer::Config>(cfg.expanded(7));
        check(bool(e), "expanded config has wrong type");
        check(e->hostName_=="node7.example", "host: "+e->hostName_);
        check(e->creationCommand_=="create-node 7", "creation command: "+e->creationCommand_);
        check(e->destructionCommand_=="destroy-node 7", "destruction command: "+e->destructionCommand_);
    });


    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
