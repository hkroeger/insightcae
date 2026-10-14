/*
 * Dynamically created / destroyed SSH servers
 * (launchCommand / shutdownCommand of SSHLinuxServer::Config).
 *
 * This is the mechanism, which will be extended for cloud instances.
 * The launch and shutdown commands are local marker scripts here.
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


    tr.run("launch command is executed for a server, which is not running", [&]()
    {
        TemporaryDirectory d;
        auto created = d.path()/"created";

        // the host is unreachable, so the launch command has to be run.
        // Only its execution is checked here (the host stays unreachable).
        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "touch '"+created.string()+"'",
            "true" );
        cfg.launchTimeout_ = 0s; // don't wait for the host

        try { cfg.instance(); } catch (...) {}
        check(fs::exists(created), "launch command was not executed");
    });


    tr.run("non-existing instance is created, and disposed if it does not become reachable", [&]()
    {
        TemporaryDirectory d;
        auto created = d.path()/"created", disposed = d.path()/"disposed";

        // the existence check fails: the creation command has to be run
        // (with the placeholders replaced). The host stays unreachable,
        // so the created instance has to be disposed again.
        SSHLinuxServer::Config cfg("/tmp", 1, "nonexistent-host.invalid");
        static_cast<std::string&>(cfg) = "cloudtest";
        cfg.creationCommand_ = "echo '%{serverLabel} %{host}' > '"+created.string()+"'";
        cfg.destructionCommand_ = "touch '"+disposed.string()+"'";
        cfg.existenceCheckCommand_ = "false";
        cfg.launchTimeout_ = 0s; // don't wait for the host

        check(cfg.exists() && !*cfg.exists(), "existence check should report a missing instance");

        expectThrows<insight::Exception>(
            [&]() { cfg.launch(); },
            "launching an instance, which does not become reachable" );

        check(fs::exists(created), "creation command was not executed");
        auto content = readFile(created);
        check(content.find("cloudtest nonexistent-host.invalid")!=std::string::npos,
              "placeholders not replaced in creation command: "+content);
        check(fs::exists(disposed), "destruction command was not executed after failed launch");
    });


    tr.run("server, which is still unreachable after creation, is reported", [&]()
    {
        TemporaryDirectory d;
        auto destroyed = d.path()/"destroyed";

        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "true",   // "creation" succeeds, but host stays unreachable
            "touch '"+destroyed.string()+"'" );
        cfg.launchTimeout_ = 10s;

        checkCompletesWithin(120s, [cfg]()
        {
            expectThrows<insight::Exception>(
                [&](){ cfg.instance(); },
                "instance() of a server, which is not reachable after creation" );
        }, "instance() of unreachable server");

        check(fs::exists(destroyed),
              "the unreachable server was not destroyed after the creation timeout");
    });


    tr.run("failing launch command is reported", [&]()
    {
        SSHLinuxServer::Config cfg(
            "/tmp", 1, "nonexistent-host.invalid",
            "exit 1",
            "true" );

        expectThrows<insight::Exception>(
            [&](){ cfg.instance(); },
            "instance() with failing launch command" );
    });


    tr.run("launch command is not executed for a running server", [&]()
    {
        TemporaryDirectory d;
        auto created = d.path()/"created";

        SSHLinuxServer::Config cfg(
            sshCfg->defaultDirectory_, 1, sshCfg->hostName_,
            "touch '"+created.string()+"'",
            "true" );

        auto srv = cfg.instance();
        check(bool(srv), "no instance");
        check(!fs::exists(created), "launch command executed although server is running");
    });


    tr.run("shutdown command is executed", [&]()
    {
        TemporaryDirectory d;
        auto destroyed = d.path()/"destroyed";

        SSHLinuxServer::Config cfg(
            sshCfg->defaultDirectory_, 1, sshCfg->hostName_,
            "true",
            "touch '"+destroyed.string()+"'" );

        auto srv = cfg.instance();
        srv->destroyIfPossible();
        check(fs::exists(destroyed), "shutdown command was not executed");
    });


    tr.run("failing shutdown command does not throw", [&]()
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
        check(e->launchCommand_=="create-node 7", "launch command: "+e->launchCommand_);
        check(e->shutdownCommand_=="destroy-node 7", "shutdown command: "+e->shutdownCommand_);
    });


    if (abandonedThreads()>0)
        tr.exit();

    return tr.finish();
}
