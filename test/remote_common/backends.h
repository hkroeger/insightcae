#ifndef INSIGHT_REMOTETEST_BACKENDS_H
#define INSIGHT_REMOTETEST_BACKENDS_H

#include <memory>
#include <string>
#include <vector>

#include "base/remoteserver.h"


namespace insight {
namespace remotetest {


/**
 * @brief The RemoteBackend class
 * describes a target, on which the backend-parametrised tests are executed.
 *
 * setUp()/tearDown() bracket the use of the backend.
 * For dynamically created targets (e.g. cloud instances, to be added later),
 * these are the places to create and destroy the instance.
 */
class RemoteBackend
{
public:
    virtual ~RemoteBackend();

    virtual std::string name() const =0;

    /**
     * @brief checkAvailability
     * @return empty string, if the backend is usable, otherwise the reason why not
     */
    virtual std::string checkAvailability() =0;

    virtual RemoteServer::ConfigPtr serverConfig() =0;

    /**
     * @brief isSSH
     * true, if the backend uses SSHLinuxServer
     */
    virtual bool isSSH() const;

    /**
     * @brief isLocalMachine
     * true, if the "remote" processes run on the machine executing the tests
     * (allows to check remote processes and ports locally)
     */
    virtual bool isLocalMachine() const;

    /**
     * @brief hasRealFileTransfer
     * true, if file transfer uses the production code (rsync)
     */
    virtual bool hasRealFileTransfer() const;

    virtual void setUp();
    virtual void tearDown();

    /**
     * @brief server
     * running server instance (created on first call)
     */
    RemoteServerPtr server();

    /**
     * @brief scratchDirectory
     * a fresh directory on the target, which is removed in tearDown()
     */
    boost::filesystem::path scratchDirectory();

    /**
     * @brief remoteOutput
     * execute command on the target and return its stdout
     */
    std::string remoteOutput(const std::string& command, int* exitCode=nullptr);

    /**
     * @brief remoteProcessesMatching
     * PIDs of processes on the target whose command line matches the (extended) regex
     */
    std::vector<int> remoteProcessesMatching(const std::string& pattern);

    bool remoteFileExists(const boost::filesystem::path& p);
    bool remoteDirectoryExists(const boost::filesystem::path& p);
    std::string remoteFileContent(const boost::filesystem::path& p);

protected:
    RemoteServerPtr server_;
    std::vector<boost::filesystem::path> scratchDirectories_;
};

typedef std::shared_ptr<RemoteBackend> RemoteBackendPtr;


/**
 * @brief The LocalShellBackend class
 * always available, uses LocalShellServer
 */
class LocalShellBackend : public RemoteBackend
{
    boost::filesystem::path baseDir_;
    RemoteServer::ConfigPtr cfg_;
public:
    LocalShellBackend();
    ~LocalShellBackend();
    std::string name() const override;
    std::string checkAvailability() override;
    RemoteServer::ConfigPtr serverConfig() override;
    bool isLocalMachine() const override;
    bool hasRealFileTransfer() const override;
};


/**
 * @brief The SSHBackend class
 * uses a SSHLinuxServer configuration
 */
class SSHBackend : public RemoteBackend
{
protected:
    std::string name_;
    RemoteServer::ConfigPtr cfg_;
    bool isLocalMachine_;
public:
    SSHBackend(const std::string& name, RemoteServer::ConfigPtr cfg, bool isLocalMachine);

    /**
     * @brief checkRemoteAnalyzeMatchesBuild
     * @return empty, if the remote analyze is the tested build, otherwise the reason
     */
    std::string checkRemoteAnalyzeMatchesBuild();
    std::string name() const override;
    std::string checkAvailability() override;
    RemoteServer::ConfigPtr serverConfig() override;
    bool isSSH() const override;
    bool isLocalMachine() const override;
};


/**
 * @brief sshLocalhostBackend
 * the server labelled "localhost" from the user's remoteservers.list.
 * Expected to be reachable by passwordless ssh.
 */
RemoteBackendPtr sshLocalhostBackend();

/**
 * @brief sshHostBackend
 * SSH host given in environment variable INSIGHT_TEST_SSH_HOST
 * (base directory in INSIGHT_TEST_SSH_BASEDIR, default /tmp)
 * @return nullptr, if not configured
 */
RemoteBackendPtr sshHostBackend();

/**
 * @brief wslBackend
 * WSL distribution given in environment variable INSIGHT_TEST_WSL_DISTRO
 * (Windows only, base directory in INSIGHT_TEST_WSL_BASEDIR, default /tmp)
 * @return nullptr, if not configured
 */
RemoteBackendPtr wslBackend();


/**
 * @brief selectBackend
 * select backend by name: "localshell", "ssh-localhost", "ssh-env", "wsl-env"
 * @return nullptr, if the backend is not configured (e.g. env var missing)
 */
RemoteBackendPtr selectBackend(const std::string& name);

/**
 * @brief backendFromCommandLine
 * parses "--backend <name>" (default: localshell)
 */
std::string backendNameFromCommandLine(int argc, char* argv[]);

/**
 * @brief availableBackendFromCommandLine
 * select the backend given by "--backend <name>" and check its availability
 * @return the backend or nullptr, if it is not configured or not available
 * (the reason is printed). The test should then exit with SKIP_RETURN_CODE.
 */
RemoteBackendPtr availableBackendFromCommandLine(int argc, char* argv[]);


/**
 * @brief The BackendSession struct
 * calls setUp() on construction and tearDown() on destruction
 */
struct BackendSession
{
    RemoteBackendPtr backend;
    BackendSession(RemoteBackendPtr be);
    ~BackendSession();
    RemoteBackend* operator->() const { return backend.get(); }
};


} // namespace remotetest
} // namespace insight

#endif // INSIGHT_REMOTETEST_BACKENDS_H
