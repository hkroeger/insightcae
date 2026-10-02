/*
 * RemoteLocation / RemoteExecutionConfig:
 * a (temporary) working directory on a remote server.
 */

#include "base/remotelocation.h"
#include "base/remoteexecution.h"
#include "base/remoteserverlist.h"

#include "backends.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;




int main(int argc, char* argv[])
{
    TestRunner tr("remote_location", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;

    {
        BackendSession session(be);

        // temporary directories created by the code under test,
        // removed at the end, if the code under test failed to do so
        std::vector<fs::path> leftovers;


        tr.run("temporary location is created on initialization", [&]()
        {
            RemoteLocation rl(be->serverConfig());
            check(!rl.isActive(), "location without directory should not be active");

            rl.initialize(true);
            leftovers.push_back(rl.remoteDir());

            check(rl.isActive(), "location should be active after initialization");
            check(!rl.remoteDir().empty(), "no remote directory assigned");
            check(rl.isTemporaryStorage(), "should be temporary storage");
            check(be->remoteDirectoryExists(rl.remoteDir()), "remote directory does not exist");
            check(rl.port()>1023, "invalid port %d", rl.port());
        });


        tr.run("existing directory is used", [&]()
        {
            auto d = be->scratchDirectory();
            RemoteLocation rl(be->serverConfig(), d);
            check(rl.isActive(), "location with existing directory should be active");
            check(!rl.isTemporaryStorage(), "should not be temporary storage");
        });


        tr.run("missing directory is created on request", [&]()
        {
            auto d = be->scratchDirectory()/"new";
            RemoteLocation rl(be->serverConfig(), d, true);
            check(!rl.isActive(), "should not be active before initialization");
            rl.initialize();
            check(rl.isActive(), "should be active after initialization");
            check(be->remoteDirectoryExists(d), "directory was not created");
        });


        tr.run("command is executed in the remote directory", [&]()
        {
            auto d = be->scratchDirectory();
            RemoteLocation rl(be->serverConfig(), d);
            rl.execRemoteCmd("pwd > pwd.txt");
            auto pwd = be->remoteFileContent(d/"pwd.txt");
            check(pwd.find(d.filename().string())!=std::string::npos,
                  "command was not executed in the remote directory: "+pwd);
        });


        tr.run("temporary location is removed by cleanup", [&]()
        {
            RemoteLocation rl(be->serverConfig());
            rl.initialize(true);
            auto rd = rl.remoteDir();
            leftovers.push_back(rd);

            rl.cleanup();
            check(!be->remoteDirectoryExists(rd), "temporary directory still exists after cleanup");
        });


        tr.run("non-temporary location is kept by cleanup", [&]()
        {
            auto d = be->scratchDirectory();
            RemoteLocation rl(be->serverConfig(), d);
            rl.cleanup();
            check(be->remoteDirectoryExists(d), "non-temporary directory was removed");
        });


        tr.run("forced cleanup removes non-temporary location", [&]()
        {
            auto d = be->scratchDirectory();
            RemoteLocation rl(be->serverConfig(), d);
            rl.cleanup(true);
            check(!be->remoteDirectoryExists(d), "directory still exists after forced cleanup");
        });


        tr.run("location is stored and restored with the local case", [&]()
        {
            std::string label = *be->serverConfig();
            try
            {
                remoteServers().findServer(label);
            }
            catch (...)
            {
                throw SkipCase("server "+label+" is not in the global server list");
            }

            TemporaryDirectory localDir;
            auto d = be->scratchDirectory();
            {
                RemoteExecutionConfig rec(be->serverConfig(), localDir.path(), d);
                check(fs::exists(localDir.path()/"meta.foam"), "meta.foam not written");
            }

            RemoteExecutionConfig rec2(localDir.path());
            check(rec2.remoteDir()==d, "restored remote directory: "+rec2.remoteDir().string());
            check(rec2.isActive(), "restored location should be active");
        });


        for (const auto& d: leftovers)
        {
            if (be->remoteDirectoryExists(d))
                be->server()->removeDirectory(d);
        }
    }

    return tr.finish();
}
