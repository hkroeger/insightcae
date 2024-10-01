#ifndef SSHLINUXSERVER_H
#define SSHLINUXSERVER_H

#include "base/linuxremoteserver.h"
#include <string>


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
    std::string creationCommand_, destructionCommand_;

    Config(
        const boost::filesystem::path& bp,
        int np,
        const std::string hostName,
        const std::string& creationCommand=std::string(),
        const std::string& destructionCommand=std::string()
        );

    Config(rapidxml::xml_node<> *e);

    std::shared_ptr<RemoteServer> instance() const override;

    //  int executeCommand(const std::string& command, bool throwOnFail) override;
    std::pair<boost::filesystem::path,std::vector<std::string> >
    commandAndArgs(const std::string& command) const override;

    bool isDynamicallyAllocated() const override;

    void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const override;

    ConfigPtr clone() const override;

    bool isExpandable() const override;
    ConfigPtr expanded(int id) const override;

    bool isDynamicallyCreatable() const override;
    bool isDynamicallyDestructable() const override;
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


  struct BackgroundJob : public RemoteServer::BackgroundJob
  {
  protected:
    int remotePid_;
  public:
    BackgroundJob(RemoteServer& server, int remotePid);
    void kill() override;
  };

  BackgroundJobPtr launchBackgroundProcess(
          const std::string& cmd,
          const std::vector<ExpectedOutput>& expectedOutputBeforeDetach = {} ) override;

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
                          std::function<void(int,const std::string&)>()
  ) override;

  void syncToLocal
  (
      const boost::filesystem::path& localDir,
      const boost::filesystem::path& remoteDir,
      bool includeProcessorDirectories,
      const std::vector<std::string>& exclude_pattern = std::vector<std::string>(),
      std::function<void(int progress,const std::string& status_text)> progress_callback =
                          std::function<void(int,const std::string&)>()
  ) override;




  struct SSHTunnelPortMapping
      : public PortMapping
  {
    boost::process::child tunnelProcess_;
    std::map<int, int> remoteToLocal_, localToRemote_;

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
