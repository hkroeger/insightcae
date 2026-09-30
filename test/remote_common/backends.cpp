#include "backends.h"
#include "localshellserver.h"
#include "remotetest.h"

#include <iostream>
#include <sstream>

#include "base/exception.h"
#include "base/remoteserverlist.h"
#include "base/sshlinuxserver.h"
#include "base/wsllinuxserver.h"

#include "boost/process.hpp"

namespace fs = boost::filesystem;
namespace bp = boost::process;


namespace insight {
namespace remotetest {




RemoteBackend::~RemoteBackend()
{}




bool RemoteBackend::isSSH() const
{
    return false;
}




bool RemoteBackend::isLocalMachine() const
{
    return false;
}




bool RemoteBackend::hasRealFileTransfer() const
{
    return true;
}




void RemoteBackend::setUp()
{}




void RemoteBackend::tearDown()
{
    if (server_)
    {
        for (const auto& d: scratchDirectories_)
        {
            try
            {
                // restore permissions possibly removed by a test
                server_->executeCommand("chmod -R u+rwx '"+toUnixPath(d)+"'", false);
                server_->removeDirectory(d);
            }
            catch (const std::exception& e)
            {
                std::cerr<<"warning: could not remove scratch directory "
                          <<d<<": "<<e.what()<<std::endl;
            }
        }
    }
    scratchDirectories_.clear();
}




RemoteServerPtr RemoteBackend::server()
{
    if (!server_)
    {
        server_ = serverConfig()->instance();
    }
    return server_;
}




boost::filesystem::path RemoteBackend::scratchDirectory()
{
    auto d = serverConfig()->defaultDirectory_ / uniqueName("insight-remotetest");
    server()->createDirectory(d);
    scratchDirectories_.push_back(d);
    return d;
}




std::string RemoteBackend::remoteOutput(const std::string& command, int* exitCode)
{
    // read the output through a temporary file:
    // avoids any pipe handling issues in the code under test
    auto tf = fs::temp_directory_path()/fs::unique_path("remoteoutput-%%%%%%%%");
    int ret = serverConfig()->executeCommand(
        command,
        bp::std_out > tf,
        bp::std_err > bp::null,
        bp::std_in < bp::null );
    std::string result = fs::exists(tf) ? readFile(tf) : std::string();
    boost::system::error_code ec;
    fs::remove(tf, ec);
    if (exitCode) *exitCode=ret;
    return result;
}




std::vector<int> RemoteBackend::remoteProcessesMatching(const std::string& pattern)
{
    std::vector<int> result;
    if (isLocalMachine())
    {
        result = localProcessesMatching(pattern);
    }
    else
    {
        // exclude the executing shell (its command line contains the pattern)
        auto out = remoteOutput(
            "for p in $(pgrep -f -- '"+pattern+"'); do "
            "if [ $p != $$ ] && [ $p != $PPID ]; then echo $p; fi; done" );
        std::istringstream is(out);
        int pid;
        while (is>>pid) result.push_back(pid);
    }
    return result;
}




bool RemoteBackend::remoteFileExists(const boost::filesystem::path& p)
{
    return serverConfig()->executeCommand(
               "test -f '"+toUnixPath(p)+"'",
               bp::std_out > bp::null, bp::std_err > bp::null, bp::std_in < bp::null )==0;
}




bool RemoteBackend::remoteDirectoryExists(const boost::filesystem::path& p)
{
    return serverConfig()->executeCommand(
               "test -d '"+toUnixPath(p)+"'",
               bp::std_out > bp::null, bp::std_err > bp::null, bp::std_in < bp::null )==0;
}




std::string RemoteBackend::remoteFileContent(const boost::filesystem::path& p)
{
    int ret;
    auto c = remoteOutput("cat '"+toUnixPath(p)+"'", &ret);
    check(ret==0, "could not read remote file "+p.string());
    return c;
}




LocalShellBackend::LocalShellBackend()
{
    // the local shell needs to find analyze and the helper scripts
    // (e.g. isPVFindPort.sh) of the build tree
    auto exe = analyzeExecutable();
    if (exe.has_parent_path())
    {
        setenv("PATH", (exe.parent_path().string()+":"+env("PATH")).c_str(), 1);
    }

    baseDir_ = fs::temp_directory_path()/fs::unique_path("insight-localshell-%%%%-%%%%");
    fs::create_directories(baseDir_);
    cfg_ = std::make_shared<LocalShellServer::Config>(baseDir_);
}




LocalShellBackend::~LocalShellBackend()
{
    boost::system::error_code ec;
    fs::remove_all(baseDir_, ec);
}




std::string LocalShellBackend::name() const
{
    return "localshell";
}




std::string LocalShellBackend::checkAvailability()
{
    if (!fs::exists("/bin/bash"))
        return "/bin/bash not found";
    return std::string();
}




RemoteServer::ConfigPtr LocalShellBackend::serverConfig()
{
    return cfg_;
}




bool LocalShellBackend::isLocalMachine() const
{
    return true;
}




bool LocalShellBackend::hasRealFileTransfer() const
{
    return false;
}




SSHBackend::SSHBackend(const std::string& name, RemoteServer::ConfigPtr cfg, bool isLocalMachine)
    : name_(name), cfg_(cfg), isLocalMachine_(isLocalMachine)
{}




std::string SSHBackend::name() const
{
    return name_;
}




std::string SSHBackend::checkAvailability()
{
    if (!cfg_)
        return "no server configuration";

    if (bp::search_path("ssh").empty())
        return "ssh executable not found in PATH";
    if (bp::search_path("rsync").empty())
        return "rsync executable not found in PATH";

    try
    {
        if (!cfg_->isRunning())
            return "server "+std::string(*cfg_)+" is not reachable by (passwordless) ssh";

        int ret;
        remoteOutput("command -v analyze", &ret);
        if (ret!=0)
            return "analyze is not in PATH on "+std::string(*cfg_);

        remoteOutput("command -v rsync", &ret);
        if (ret!=0)
            return "rsync is not in PATH on "+std::string(*cfg_);

        remoteOutput("command -v isPVFindPort.sh", &ret);
        if (ret!=0)
            return "isPVFindPort.sh is not in PATH on "+std::string(*cfg_);

        auto bd = toUnixPath(cfg_->defaultDirectory_);
        remoteOutput("mkdir -p '"+bd+"' && test -w '"+bd+"'", &ret);
        if (ret!=0)
            return "base directory "+bd+" is not writable on "+std::string(*cfg_);
    }
    catch (const std::exception& e)
    {
        return std::string("error while checking server: ")+e.what();
    }
    return std::string();
}




RemoteServer::ConfigPtr SSHBackend::serverConfig()
{
    return cfg_;
}




bool SSHBackend::isSSH() const
{
    return true;
}




bool SSHBackend::isLocalMachine() const
{
    return isLocalMachine_;
}




class UnavailableBackend : public RemoteBackend
{
    std::string name_, reason_;
public:
    UnavailableBackend(const std::string& name, const std::string& reason)
        : name_(name), reason_(reason)
    {}
    std::string name() const override { return name_; }
    std::string checkAvailability() override { return reason_; }
    RemoteServer::ConfigPtr serverConfig() override
    {
        throw insight::Exception("backend %s is not available", name_.c_str());
    }
};




RemoteBackendPtr sshLocalhostBackend()
{
    RemoteServer::ConfigPtr cfg;
    try
    {
        cfg = remoteServers().findServer("localhost");
    }
    catch (const std::exception& e)
    {
        return std::make_shared<UnavailableBackend>(
            "ssh-localhost",
            "no remote server labelled \"localhost\" configured in remoteservers.list" );
    }

    if (!std::dynamic_pointer_cast<SSHLinuxServer::Config>(cfg))
    {
        return std::make_shared<UnavailableBackend>(
            "ssh-localhost",
            "remote server \"localhost\" is not of type SSHLinux" );
    }

    return std::make_shared<SSHBackend>("ssh-localhost", cfg, true);
}




RemoteBackendPtr sshHostBackend()
{
    auto host = env("INSIGHT_TEST_SSH_HOST");
    if (host.empty())
        return nullptr;

    auto cfg = std::make_shared<SSHLinuxServer::Config>(
        env("INSIGHT_TEST_SSH_BASEDIR", "/tmp"), 1, host );
    static_cast<std::string&>(*cfg)="ssh-env";

    return std::make_shared<SSHBackend>(
        "ssh-env", cfg,
        host=="localhost" || host=="127.0.0.1" );
}




class WSLBackend : public RemoteBackend
{
    RemoteServer::ConfigPtr cfg_;
public:
    WSLBackend(RemoteServer::ConfigPtr cfg) : cfg_(cfg) {}
    std::string name() const override { return "wsl-env"; }
    std::string checkAvailability() override
    {
        try
        {
            if (!cfg_->isRunning())
                return "WSL distribution "+std::string(*cfg_)+" is not usable";
            int ret;
            remoteOutput("command -v analyze", &ret);
            if (ret!=0)
                return "analyze is not in PATH in WSL distribution";
        }
        catch (const std::exception& e)
        {
            return std::string("error while checking WSL: ")+e.what();
        }
        return std::string();
    }
    RemoteServer::ConfigPtr serverConfig() override { return cfg_; }
};




RemoteBackendPtr wslBackend()
{
    auto distro = env("INSIGHT_TEST_WSL_DISTRO");
    if (distro.empty())
        return nullptr;

#ifdef WIN32
    auto cfg = std::make_shared<WSLLinuxServer::Config>(
        env("INSIGHT_TEST_WSL_BASEDIR", "/tmp"), 1, distro );
    static_cast<std::string&>(*cfg)="wsl-env";
    return std::make_shared<WSLBackend>(cfg);
#else
    return std::make_shared<UnavailableBackend>(
        "wsl-env", "WSL backend is only available on Windows" );
#endif
}




RemoteBackendPtr selectBackend(const std::string& name)
{
    if (name=="localshell")
        return std::make_shared<LocalShellBackend>();
    else if (name=="ssh-localhost")
        return sshLocalhostBackend();
    else if (name=="ssh-env")
        return sshHostBackend();
    else if (name=="wsl-env")
        return wslBackend();

    throw insight::Exception("unknown backend: %s", name.c_str());
}




std::string backendNameFromCommandLine(int argc, char* argv[])
{
    for (int i=1; i<argc-1; ++i)
    {
        if (std::string(argv[i])=="--backend")
            return argv[i+1];
    }
    return "localshell";
}




RemoteBackendPtr availableBackendFromCommandLine(int argc, char* argv[])
{
    auto name = backendNameFromCommandLine(argc, argv);
    auto be = selectBackend(name);
    if (!be)
    {
        std::cout<<"backend "<<name<<" is not configured: skipping"<<std::endl;
        return nullptr;
    }
    auto reason = be->checkAvailability();
    if (!reason.empty())
    {
        std::cout<<"backend "<<name<<" is not available: "<<reason<<std::endl;
        return nullptr;
    }
    std::cout<<"using backend "<<be->name()<<std::endl;
    return be;
}




BackendSession::BackendSession(RemoteBackendPtr be)
    : backend(be)
{
    backend->setUp();
}




BackendSession::~BackendSession()
{
    try
    {
        backend->tearDown();
    }
    catch (const std::exception& e)
    {
        std::cerr<<"warning: tear down of backend failed: "<<e.what()<<std::endl;
    }
}




} // namespace remotetest
} // namespace insight
