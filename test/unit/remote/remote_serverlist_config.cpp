/*
 * Reading and writing of the remote server configuration
 * (remoteservers.list) and of the per-case remote location file (meta.foam).
 *
 * Hermetic: the shared path list is redirected to temporary directories.
 * No server is contacted.
 */

#include <iostream>

#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/remoteserverlist.h"
#include "base/remotelocation.h"
#include "base/sharedpathlist.h"
#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"

#include "remotetest.h"


using namespace insight;
using namespace insight::remotetest;
namespace fs = boost::filesystem;




struct ConfigDirs
{
    TemporaryDirectory userDir{"remotetest-user"}, globalDir{"remotetest-global"};

    ConfigDirs()
    {
        // the server list is read in reverse order:
        // first the global directories, then the user directory (which overrides)
        auto& spl = SharedPathList::global();
        spl.clear();
        spl.push_back(userDir.path());
        spl.push_back(globalDir.path());
    }

    void writeUser(const std::string& content)
    {
        writeFile(userDir.path()/"remoteservers.list", content);
    }

    void writeGlobal(const std::string& content)
    {
        writeFile(globalDir.path()/"remoteservers.list", content);
    }

    void clear()
    {
        fs::remove(userDir.path()/"remoteservers.list");
        fs::remove(globalDir.path()/"remoteservers.list");
    }
};




template<class C>
std::shared_ptr<C> findAs(const RemoteServerList& l, const std::string& label)
{
    auto c = std::dynamic_pointer_cast<C>(l.findServer(label));
    check(bool(c), "server "+label+" has unexpected type");
    return c;
}




int countLabel(const RemoteServerList& l, const std::string& label)
{
    int n=0;
    for (const auto& s: l)
        if (static_cast<const std::string&>(*s)==label) ++n;
    return n;
}




int main(int argc, char* argv[])
{
    TestRunner tr("remote_serverlist_config", argc, argv);

    ConfigDirs dirs;


    tr.run("parse SSH server entry", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"srv1\" type=\"SSHLinux\" host=\"host1.example\" baseDirectory=\"/scratch/x\" np=\"8\"/>"
            "<remoteServer label=\"srv2\" host=\"host2.example\" baseDirectory=\"/scratch/y\"/>"
            "<remoteServer label=\"srv3\" type=\"SSHLinux\" host=\"host3.example\" baseDirectory=\"/scratch/z\""
            "   creationCommand=\"create %d\" destructionCommand=\"destroy %d\"/>"
            "</root>" );

        RemoteServerList l;
        check(l.size()==3, "expected 3 servers, got %d", int(l.size()));

        auto s1 = findAs<SSHLinuxServer::Config>(l, "srv1");
        check(s1->hostName_=="host1.example", "host of srv1: "+s1->hostName_);
        check(s1->defaultDirectory_=="/scratch/x", "base directory of srv1");
        check(s1->np_==8, "np of srv1: %d", s1->np_);

        // missing type means SSH
        auto s2 = findAs<SSHLinuxServer::Config>(l, "srv2");
        check(s2->np_==1, "default np of srv2: %d", s2->np_);

        auto s3 = findAs<SSHLinuxServer::Config>(l, "srv3");
        check(s3->isDynamicallyCreatable() && s3->isDynamicallyDestructable(),
              "srv3 should be dynamically creatable and destructable");
    });


    tr.run("parse WSL server entry", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"wsl1\" type=\"WSLLinux\" distributionLabel=\"insightcae-ubuntu\" baseDirectory=\"/home/u/runs\" np=\"4\"/>"
            "</root>" );

        RemoteServerList l;
        auto w = findAs<WSLLinuxServer::Config>(l, "wsl1");
        check(w->distributionLabel_=="insightcae-ubuntu", "distribution label: "+w->distributionLabel_);
        check(w->defaultDirectory_=="/home/u/runs", "base directory");
        check(w->np_==4, "np: %d", w->np_);
    });


    tr.run("SSH server np survives save/load round trip", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"srv1\" type=\"SSHLinux\" host=\"host1.example\" baseDirectory=\"/scratch/x\" np=\"8\"/>"
            "</root>" );

        TemporaryDirectory out;
        {
            RemoteServerList l;
            l.writeConfiguration(out.path()/"remoteservers.list");
        }
        fs::copy_file(out.path()/"remoteservers.list", dirs.userDir.path()/"remoteservers.list",
                      fs::copy_option::overwrite_if_exists);

        RemoteServerList l2;
        auto s1 = findAs<SSHLinuxServer::Config>(l2, "srv1");
        check(s1->np_==8, "np after round trip is %d, expected 8 (np is not saved)", s1->np_);
    });


    tr.run("WSL server np survives save/load round trip", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"wsl1\" type=\"WSLLinux\" distributionLabel=\"d\" baseDirectory=\"/b\" np=\"4\"/>"
            "</root>" );

        TemporaryDirectory out;
        {
            RemoteServerList l;
            l.writeConfiguration(out.path()/"remoteservers.list");
        }
        fs::copy_file(out.path()/"remoteservers.list", dirs.userDir.path()/"remoteservers.list",
                      fs::copy_option::overwrite_if_exists);

        RemoteServerList l2;
        auto w = findAs<WSLLinuxServer::Config>(l2, "wsl1");
        check(w->np_==4, "np after round trip is %d, expected 4 (np is not saved)", w->np_);
    });


    tr.run("user entry overrides global entry with same label", [&]()
    {
        dirs.clear();
        dirs.writeGlobal(
            "<root>"
            "<remoteServer label=\"srv\" type=\"SSHLinux\" host=\"globalhost\" baseDirectory=\"/g\"/>"
            "<remoteServer label=\"onlyglobal\" type=\"SSHLinux\" host=\"g2\" baseDirectory=\"/g\"/>"
            "</root>" );
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"srv\" type=\"SSHLinux\" host=\"userhost\" baseDirectory=\"/u\"/>"
            "</root>" );

        RemoteServerList l;
        check(countLabel(l, "onlyglobal")==1, "global-only entry missing");
        int n = countLabel(l, "srv");
        check(n==1, "expected exactly one entry labelled srv, got %d", n);
        auto s = findAs<SSHLinuxServer::Config>(l, "srv");
        check(s->hostName_=="userhost",
              "entry srv should come from the user configuration, but has host "+s->hostName_);
    });


    tr.run("duplicate label within one file: first definition is used", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"srv\" type=\"SSHLinux\" host=\"first\" baseDirectory=\"/a\"/>"
            "<remoteServer label=\"srv\" type=\"SSHLinux\" host=\"second\" baseDirectory=\"/b\"/>"
            "</root>" );

        RemoteServerList l;
        int n = countLabel(l, "srv");
        check(n==1, "expected exactly one entry labelled srv, got %d", n);
        auto s = findAs<SSHLinuxServer::Config>(l, "srv");
        check(s->hostName_=="first", "expected the first definition, got host "+s->hostName_);
    });


    tr.run("server pool is expanded and saved as pool", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServerPool label=\"pool\" type=\"SSHLinux\" host=\"node%d\" baseDirectory=\"/p\" maxSize=\"3\" np=\"2\"/>"
            "</root>" );

        RemoteServerList l;
        check(l.serverPools().size()==1, "expected one pool");
        for (int i=1; i<=3; ++i)
        {
            auto lbl = "pool/node"+std::to_string(i);
            auto s = findAs<SSHLinuxServer::Config>(l, lbl);
            check(s->wasExpanded(), lbl+" should be marked as expanded");
        }

        TemporaryDirectory out;
        l.writeConfiguration(out.path()/"remoteservers.list");
        auto content = readFile(out.path()/"remoteservers.list");
        check(content.find("node1")==std::string::npos,
              "expanded pool members must not be written:\n"+content);
        check(content.find("remoteServerPool")!=std::string::npos,
              "pool must be written:\n"+content);
    });


    tr.run("copy of server list keeps pools", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServerPool label=\"pool\" type=\"SSHLinux\" host=\"node%d\" baseDirectory=\"/p\" maxSize=\"2\" np=\"2\"/>"
            "</root>" );

        RemoteServerList l;
        RemoteServerList copy(l);
        check(copy.size()==l.size(), "copy has different number of servers");
        check(copy.serverPools().size()==l.serverPools().size(),
              "copy has %d pools, original %d",
              int(copy.serverPools().size()), int(l.serverPools().size()));
    });


    tr.run("WSL entry without distributionLabel is rejected with an exception", [&]()
    {
        checkIsolated([]()
        {
            XMLDocument doc;
            auto *e = doc.allocate_node(rapidxml::node_element, "remoteServer");
            e->append_attribute(doc.allocate_attribute("label", "wsl"));
            e->append_attribute(doc.allocate_attribute("type", "WSLLinux"));
            e->append_attribute(doc.allocate_attribute("baseDirectory", "/b"));
            expectThrows<insight::Exception>(
                [&]() { RemoteServer::Config::create(e); },
                "creating WSL config without distributionLabel" );
        }, "WSL entry without distributionLabel");
    });


    tr.run("WSL entry without baseDirectory is rejected with an exception", [&]()
    {
        checkIsolated([]()
        {
            XMLDocument doc;
            auto *e = doc.allocate_node(rapidxml::node_element, "remoteServer");
            e->append_attribute(doc.allocate_attribute("label", "wsl"));
            e->append_attribute(doc.allocate_attribute("type", "WSLLinux"));
            e->append_attribute(doc.allocate_attribute("distributionLabel", "d"));
            expectThrows<insight::Exception>(
                [&]() { RemoteServer::Config::create(e); },
                "creating WSL config without baseDirectory" );
        }, "WSL entry without baseDirectory");
    });


    tr.run("invalid entry is skipped, other entries remain usable", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"good\" type=\"SSHLinux\" host=\"h\" baseDirectory=\"/b\"/>"
            "<remoteServer label=\"bad\" type=\"SSHLinux\" baseDirectory=\"/b\"/>" // no host
            "</root>" );

        checkIsolated([]()
        {
            RemoteServerList l;
            check(countLabel(l, "good")==1, "valid entry missing");
            check(countLabel(l, "bad")==0, "invalid entry should have been skipped");
        }, "reading list with an invalid entry");
    });


    tr.run("broken server list file does not prevent reading the others", [&]()
    {
        dirs.clear();
        dirs.writeGlobal(
            "<root>"
            "<remoteServer label=\"good\" type=\"SSHLinux\" host=\"h\" baseDirectory=\"/b\"/>"
            "</root>" );
        dirs.writeUser("<notroot/>");

        checkIsolated([]()
        {
            RemoteServerList l;
            check(countLabel(l, "good")==1,
                  "the valid global configuration should still be available");
        }, "reading server lists with one broken file");
    });


    tr.run("remote location file (meta.foam) round trip", [&]()
    {
        dirs.clear();
        dirs.writeUser(
            "<root>"
            "<remoteServer label=\"testsrv\" type=\"SSHLinux\" host=\"nonexistent.invalid\" baseDirectory=\"/b\"/>"
            "</root>" );

        // the global list is read on first use
        check(remoteServers().findServer("testsrv")!=nullptr, "server not found in global list");

        TemporaryDirectory d;
        writeFile(d.path()/"meta.foam",
            "<remote server=\"testsrv\" temporary=\"yes\" autocreate=\"no\""
            " directory=\"/b/ir123456\" port=\"12345\"/>" );

        RemoteLocation rl(d.path()/"meta.foam", true);
        check(rl.serverLabel()=="testsrv", "server label: "+rl.serverLabel());
        check(rl.remoteDir()=="/b/ir123456", "remote dir: "+rl.remoteDir().string());
        check(rl.isTemporaryStorage(), "should be temporary storage");
        // port() asserts a validated location, so check the written file instead
        rl.writeConfigFile(d.path()/"meta2.foam");

        RemoteLocation rl2(d.path()/"meta2.foam", true);
        check(rl2.serverLabel()=="testsrv", "server label after round trip");
        check(rl2.remoteDir()=="/b/ir123456", "remote dir after round trip");
        check(rl2.isTemporaryStorage(), "temporary flag after round trip");

        auto c = readFile(d.path()/"meta2.foam");
        check(c.find("12345")!=std::string::npos, "port not written:\n"+c);
    });


    tr.run("remote location file with unknown server gives a clear error", [&]()
    {
        TemporaryDirectory d;
        writeFile(d.path()/"meta.foam",
            "<remote server=\"doesnotexist\" temporary=\"yes\" autocreate=\"no\" directory=\"/b/x\"/>" );

        auto msg = expectThrows<insight::Exception>(
            [&]() { RemoteLocation rl(d.path()/"meta.foam", true); },
            "reading remote location with unknown server" );
        check(msg.find("doesnotexist")!=std::string::npos,
              "error message should name the unknown server: "+msg);
    });


    return tr.finish();
}
