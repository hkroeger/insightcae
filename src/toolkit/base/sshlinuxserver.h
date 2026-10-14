#ifndef SSHLINUXSERVER_H
#define SSHLINUXSERVER_H

#include "base/linuxremoteserver.h"
#include <string>
#include <chrono>


namespace insight
{


/**
 * @brief The SSHLinuxServer class
 * Server with ssh based command execution and rsync based file transfer
 */
class SSHLinuxServer
    : public LinuxRemoteServer
{
public:
  struct Config : public LinuxRemoteServer::Config
  {
    std::string hostName_;

    /**
     * commands for hosts, which are launched on demand (e.g. virtual machines),
     * executed locally. Both or none have to be given.
     */
    std::string launchCommand_, shutdownCommand_;

    /**
     * commands for instances, which have to be created and disposed
     * (e.g. cloud instances), executed locally. All or none have to be given.
     * The existence check command has to exit with code 0, if the instance exists.
     */
    std::string creationCommand_, destructionCommand_, existenceCheckCommand_;

    /**
     * In all commands, the placeholders %{serverLabel} and %{host} are replaced
     * by the server label and the host name before execution.
     */
    std::string expandPlaceholders(const std::string& command) const;

    /**
     * time to wait, until a launched or created server becomes reachable
     * (e.g. boot time of a cloud instance)
     */
    std::chrono::seconds launchTimeout_ = defaultLaunchTimeout();
    static std::chrono::seconds defaultLaunchTimeout();

    Config(
        const boost::filesystem::path& bp,
        int np,
        const std::string hostName,
        const std::string& launchCommand=std::string(),
        const std::string& shutdownCommand=std::string()
        );

    Config(rapidxml::xml_node<> *e);

    std::shared_ptr<RemoteServer> instance() const override;

    //  int executeCommand(const std::string& command, bool throwOnFail) override;
    std::pair<boost::filesystem::path,std::vector<std::string> >
    commandAndArgs(const std::string& command) const override;

    /**
     * probe with a short connect timeout, so that an unreachable host
     * does not block the caller for the OS TCP timeout
     */
    bool isRunning() const override;

    bool isDynamicallyAllocated() const override;

    void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const override;

    ConfigPtr clone() const override;

    bool isExpandable() const override;
    ConfigPtr expanded(int id) const override;

    bool canBeLaunched() const override;
    bool canBeShutDown() const override;
    bool canBeCreated() const override;
    bool canBeDisposed() const override;

    void launch(OutputLineCallback outputLine = OutputLineCallback()) const override;
    void shutdown(OutputLineCallback outputLine = OutputLineCallback()) const override;
    boost::optional<bool> exists() const override;
    void dispose(OutputLineCallback outputLine = OutputLineCallback()) const override;

  private:
    /**
     * execute a command locally after placeholder expansion
     * @return exit code
     */
    int runLocalCommand(const std::string& command, OutputLineCallback outputLine) const;

    /**
     * @return false, if the server did not become reachable within the launch timeout
     */
    bool waitUntilReachable(OutputLineCallback outputLine) const;

  public:

    /**
     * @brief runShutdownCommand
     * execute the shutdown command, if there is one. Does not throw.
     */
    void runShutdownCommand(OutputLineCallback outputLine = OutputLineCallback()) const;
  };


protected:
  Config serverConfig_;
  int bwlimit_;

  void runRsync
  (
      const std::vector<std::string>& args,
      std::function<void(int,const std::string&)> pf
  );


public:
  SSHLinuxServer(const Config& serverConfig);


  void destroyIfPossible() override;

  const RemoteServer::Config& config() const override;
  const Config& SSHServerConfig() const;
  std::string hostName() const;


  void putFile
  (
      const boost::filesystem::path& localFilePath,
      const boost::filesystem::path& remoteFilePath,
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) override;

  void setTransferBandWidthLimit(int kBPerSecond) override;
  int transferBandWidthLimit() const override;

  void syncToRemote
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      bool includeProcessorDirectories,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>(),
      const std::vector<boost::filesystem::path>& selection =
                          std::vector<boost::filesystem::path>()
  ) override;

  void syncToLocal
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      bool includeProcessorDirectories,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>(),
      const std::vector<boost::filesystem::path>& selection =
                          std::vector<boost::filesystem::path>()
  ) override;




  struct SSHTunnelPortMapping
      : public PortMapping
  {
    boost::process::child tunnelProcess_;
    std::map<int, int> remoteToLocal_, localToRemote_;
    boost::filesystem::path errorLog_; // stderr of the ssh tunnel process

    /**
     * @brief waitUntilReady
     * wait until the local ends of the tunnels accept connections.
     * Throws, if ssh exits or the tunnels are not ready within the timeout.
     */
    void waitUntilReady(std::chrono::milliseconds timeout);

  public:
    int localListenerPort(int remoteListenerPort) const override;
    int remoteListenerPort(int localListenerPort) const override;

    SSHTunnelPortMapping(
        const Config& cfg,
        const std::set<int>& remoteListenerPorts,
        const std::set<int>& localListenerPorts
        );
    /**
     * @brief ~RemotePorts
     * tunnels are closed in destructor
     */
    ~SSHTunnelPortMapping();
  };

  /**
   * @brief makePortsAccessible
   * create tunnels to remote location
   * @param remoteListenPorts
   * Will be translated to a subset of SSH's -R option:
   * Specifies that connections to the given TCP port or Unix socket on the remote (server) host are to be
   * forwarded to the local side.
   * This works by allocating a socket to listen to either a TCP port or to a Unix socket on the remote side.
   * -R localport:host:hostport
   * The local port will be chosen automatically
   *
   * @param localListenPorts
   * Will be translated to a subset of SSH's -L option:
   * Specifies that connections to the given TCP port or Unix socket on the local (client) host are to be
   * forwarded to the given host and port, or Unix socket, on the remote side.
   * -L localport:host:hostport
   * The remote port will be chosen automatically
   *
   */
   PortMappingPtr makePortsAccessible(
       const std::set<int>& remoteListenerPorts,
       const std::set<int>& localListenerPorts
       ) override;

};



}


#endif // SSHLINUXSERVER_H
