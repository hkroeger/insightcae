/*
 * File system operations and command execution of the remote server classes
 * (LinuxRemoteServer and derived), executed on the backend given by --backend.
 */

#include <future>
#include <set>

#include "boost/process.hpp"

#include "base/linuxremoteserver.h"
#include "base/remotecommand.h"

#include "backends.h"
#include "fakeanalyzeserver.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace bp = boost::process;
namespace fs = boost::filesystem;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_server_fileops", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;
    {
    BackendSession session(be);


    tr.run("command exit codes are reported", [&]()
    {
        auto srv = be->server();
        check(srv->executeCommand("true", false)==0, "true should return 0");
        check(srv->executeCommand("exit 3", false)==3, "exit code 3 expected");
        expectThrows<insight::Exception>(
            [&](){ srv->executeCommand("exit 3", true); },
            "executeCommand with throwOnFail" );
    });


    tr.run("create, detect and remove directory", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        auto d = base/"sub1"/"sub2";

        check(!srv->checkIfDirectoryExists(d), "directory should not exist yet");
        srv->createDirectory(d);
        check(srv->checkIfDirectoryExists(d), "directory should exist after creation");
        check(be->remoteDirectoryExists(d), "directory not found by independent check");

        srv->removeDirectory(base/"sub1");
        check(!srv->checkIfDirectoryExists(d), "directory should be removed");
    });


    tr.run("list directory contents", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        srv->createDirectory(base/"dirA");
        srv->createDirectory(base/"dirB");
        srv->executeCommand("touch '"+toUnixPath(base/"file1")+"'", true);

        auto ls = srv->listRemoteDirectory(base);
        std::set<std::string> names;
        for (const auto& e: ls) names.insert(e.filename().string());
        check(names==std::set<std::string>({"dirA", "dirB", "file1"}),
              "unexpected directory listing (%d entries)", int(names.size()));

        auto sd = srv->listRemoteSubdirectories(base);
        std::set<std::string> subdirs;
        for (const auto& e: sd) subdirs.insert(e.filename().string());
        check(subdirs==std::set<std::string>({"dirA", "dirB"}),
              "unexpected subdirectory listing (%d entries)", int(subdirs.size()));
    });


    tr.run("listing a missing directory is reported", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        expectThrows<insight::Exception>(
            [&](){ srv->listRemoteDirectory(base/"doesnotexist"); },
            "listing a nonexistent directory" );
    });


    tr.run("paths with blanks", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        auto d = base/"dir with blanks";
        srv->createDirectory(d);
        check(srv->checkIfDirectoryExists(d), "directory with blanks not found");
        srv->createDirectory(d/"sub dir");

        auto sd = srv->listRemoteSubdirectories(d);
        check(sd.size()==1 && sd[0].filename()=="sub dir",
              "subdirectory of a path with blanks not listed (%d entries)", int(sd.size()));

        srv->removeDirectory(d);
        check(!srv->checkIfDirectoryExists(d), "directory with blanks not removed");
    });


    tr.run("paths with shell special characters", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        // must not be interpreted by the remote shell
        auto d = base/"dir_$HOME_`id`_'q'_\"dq\"";
        srv->createDirectory(d);

        auto ls = srv->listRemoteDirectory(base);
        check(ls.size()==1 && ls[0].filename()==d.filename(),
              "directory name was altered by the remote shell: %s",
              ls.size()==1 ? ls[0].string().c_str() : "(no single entry)");
        check(srv->checkIfDirectoryExists(d), "directory with special characters not found");
        srv->removeDirectory(d);
        check(!srv->checkIfDirectoryExists(d), "directory with special characters not removed");
    });


    tr.run("temporary directory names are unique and reserved", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();

        // at most 8 calls in parallel: each call opens ssh connections one after another,
        // and sshd drops connections beyond 10 pending ones by default (MaxStartups 10:30:100)
        const int nParallel=8, nRounds=3, nTotal=nParallel*nRounds;

        std::set<fs::path> unique;
        for (int round=0; round<nRounds; ++round)
        {
            std::vector<std::future<fs::path> > names;
            for (int i=0; i<nParallel; ++i)
            {
                names.push_back(std::async(std::launch::async, [srv,base]()
                {
                    return srv->getTemporaryDirectoryName(base/"irXXXXXX");
                }));
            }
            for (auto& n: names) unique.insert(n.get());
        }
        check(int(unique.size())==nTotal,
              "only %d unique names out of %d", int(unique.size()), nTotal);

        // the name must be reserved on the remote side,
        // otherwise two runs could pick the same directory
        int nExisting=0;
        for (const auto& n: unique)
            if (be->remoteDirectoryExists(n)) ++nExisting;
        check(nExisting==nTotal,
              "%d of %d temporary directories exist on the remote side "
              "(name is not reserved by creating it)", nExisting, nTotal);
    });


    tr.run("remote output stream writes file content", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();

        std::string content;
        for (int i=0; i<5000; ++i) content+="line "+std::to_string(i)+" of the input file\n";

        {
            auto rs = srv->remoteOFStream(base/"param.ist", content.size());
            rs->stream() << content;
            rs->close();
        }

        check(be->remoteFileContent(base/"param.ist")==content,
              "remote file content differs");
    });


    tr.run("remote output stream to an unwritable location reports an error", [&]()
    {
        auto base = be->scratchDirectory();
        auto target = base/"doesnotexist"/"param.ist";
        auto cfg = be->serverConfig();

        // run isolated: writing into a pipe, whose reader has already exited,
        // may raise SIGPIPE
        checkIsolated([cfg,target]()
        {
            auto srv = cfg->instance();
            expectThrows<insight::Exception>(
                [&]()
                {
                    auto rs = srv->remoteOFStream(target, 10);
                    rs->stream() << "some content\n";
                    rs->close();
                },
                "writing to a remote file in a nonexistent directory" );
        }, "remote output stream error reporting");
    });


    tr.run("command with large output does not deadlock", [&]()
    {
        // output must be consumed while the command runs
        auto cfg = be->serverConfig();
        checkCompletesWithin(60s, [cfg]()
        {
            auto r = cfg->runCommand("seq 1 200000; seq 1 100000 >&2");
            check(r.exitCode==0, "command failed");
            std::istringstream is(r.out);
            std::string line, last;
            while (std::getline(is, line)) last=line;
            check(last=="200000", "unexpected last line of stdout: "+last);
            check(r.err.size()>500000, "stderr not captured completely");
        }, "command with 1.3 MB of output");
    });


    tr.run("directory listing with large error output does not deadlock", [&]()
    {
        auto srv = be->server();
        auto base = be->scratchDirectory();
        // many entries on stdout
        srv->executeCommand(
            "cd '"+toUnixPath(base)+"' && for i in $(seq 1 3000); do touch f_$i; done", true);

        checkCompletesWithin(60s, [srv,base]()
        {
            auto ls = srv->listRemoteDirectory(base);
            check(ls.size()==3000, "expected 3000 entries, got %d", int(ls.size()));
        }, "listing 3000 files");
    });


    tr.run("free remote port is free", [&]()
    {
        auto srv = be->server();
        int p = srv->findFreeRemotePort();
        check(p>1023 && p<65536, "invalid port %d", p);

        if (be->isLocalMachine())
        {
            // occupy the port and ask again: must not get the same port
            PortBlocker blocker(p);
            int p2 = srv->findFreeRemotePort();
            check(p2!=p, "port %d is reported free although it is in use", p);
        }
        else
        {
            throw SkipCase("occupation check requires a local backend");
        }
    });


    } // tear down backend

    if (abandonedThreads()>0)
        tr.exit(); // hanging threads: don't wait for them

    return tr.finish();
}
