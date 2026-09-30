/*
 * File transfer between the local machine and the remote server
 * (rsync based in SSHLinuxServer and WSLLinuxServer).
 */

#include "base/linuxremoteserver.h"

#include "backends.h"
#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
using namespace std::chrono_literals;
namespace fs = boost::filesystem;




void createCase(const fs::path& d)
{
    writeFile(d/"system"/"controlDict", "application simpleFoam;\n");
    writeFile(d/"constant"/"transportProperties", "nu 1e-5;\n");
    writeFile(d/"0"/"U", "internalField uniform (1 0 0);\n");
    writeFile(d/"processor0"/"0"/"U", "decomposed\n");
    writeFile(d/"file with blanks.txt", "blanks\n");
    std::string big;
    for (int i=0; i<100000; ++i) big+="0123456789";
    writeFile(d/"big.dat", big);
}




int main(int argc, char* argv[])
{
    TestRunner tr("remote_server_sync", argc, argv);

    auto be = availableBackendFromCommandLine(argc, argv);
    if (!be) return SKIP_RETURN_CODE;
    if (!be->hasRealFileTransfer())
    {
        std::cout<<"backend "<<be->name()<<" has no production file transfer: skipping"<<std::endl;
        return SKIP_RETURN_CODE;
    }

    {
        BackendSession session(be);


        tr.run("round trip of a case directory", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src, dst;
            createCase(src);
            auto rd = be->scratchDirectory()/"case";

            srv->syncToRemote(src, rd, true);
            check(be->remoteFileContent(rd/"system"/"controlDict")=="application simpleFoam;\n",
                  "file content on remote side differs");

            srv->syncToLocal(dst, rd, true);
            auto diff = compareDirectories(src, dst);
            check(diff.empty(), "directories differ after round trip: "+diff);
        });


        tr.run("processor directories are excluded on request", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            createCase(src);
            auto rd = be->scratchDirectory()/"case";

            srv->syncToRemote(src, rd, false);
            check(!be->remoteDirectoryExists(rd/"processor0"),
                  "processor directory should not be transferred");
            check(be->remoteFileExists(rd/"0"/"U"), "regular file missing");
        });


        tr.run("additional exclude patterns", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            createCase(src);
            auto rd = be->scratchDirectory()/"case";

            srv->syncToRemote(src, rd, true, {"*.dat"});
            check(!be->remoteFileExists(rd/"big.dat"), "excluded file was transferred");
            check(be->remoteFileExists(rd/"file with blanks.txt"), "file with blanks missing");
        });


        tr.run("progress is reported", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            createCase(src);
            auto rd = be->scratchDirectory()/"case";

            int nCalls=0, lastProgress=-1;
            srv->syncToRemote(src, rd, true, {},
                [&](int progress, const std::string&)
                {
                    ++nCalls;
                    lastProgress=progress;
                });
            check(nCalls>0, "no progress reported");
            check(lastProgress>=0 && lastProgress<=100, "invalid progress value %d", lastProgress);
        });


        tr.run("single file transfer", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            writeFile(src.path()/"param.ist", "<root/>\n");
            auto rd = be->scratchDirectory();

            srv->putFile(src.path()/"param.ist", rd/"param.ist");
            check(be->remoteFileContent(rd/"param.ist")=="<root/>\n", "file content differs");
        });


        tr.run("upload into an unwritable remote directory is reported", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            createCase(src);
            auto rd = be->scratchDirectory()/"readonly";
            srv->createDirectory(rd);
            srv->executeCommand("chmod a-w '"+toUnixPath(rd)+"'", true);

            expectThrows<insight::Exception>(
                [&](){ srv->syncToRemote(src, rd/"case", true); },
                "upload into an unwritable remote directory" );
        });


        tr.run("download of a missing remote directory is reported", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory dst;
            auto rd = be->scratchDirectory()/"doesnotexist";

            expectThrows<insight::Exception>(
                [&](){ srv->syncToLocal(dst, rd, true); },
                "download of a nonexistent remote directory" );
        });


        tr.run("download into an unwritable local directory is reported", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src, dst;
            createCase(src);
            auto rd = be->scratchDirectory()/"case";
            srv->syncToRemote(src, rd, true);

            fs::permissions(dst.path(), fs::owner_read|fs::owner_exe);
            expectThrows<insight::Exception>(
                [&](){ srv->syncToLocal(dst.path()/"case", rd, true); },
                "download into an unwritable local directory" );
            fs::permissions(dst.path(), fs::owner_all);
        });


        tr.run("transfer of a missing local file is reported", [&]()
        {
            auto srv = be->server();
            TemporaryDirectory src;
            auto rd = be->scratchDirectory();

            expectThrows<insight::Exception>(
                [&](){ srv->putFile(src.path()/"doesnotexist.ist", rd/"param.ist"); },
                "transfer of a nonexistent local file" );
        });
    }

    return tr.finish();
}
