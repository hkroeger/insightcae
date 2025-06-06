#ifndef INSIGHT_WSLLINUXSERVER_H
#define INSIGHT_WSLLINUXSERVER_H

#include "base/linuxremoteserver.h"
#include "toolkitversion.h"

namespace insight {

/**
 * @brief The WSLLinuxServer class
 * local WSL instance (not on a remote machine!)
 */
class WSLLinuxServer
    : public LinuxRemoteServer
{
public:
  struct Config : public LinuxRemoteServer::Config
  {
    std::string distributionLabel_;

    Config(
        const boost::filesystem::path& bp,
        int np,
        const std::string& distributionLabel );

    Config(rapidxml::xml_node<> *e);

    std::shared_ptr<RemoteServer> instance() const override;

    std::pair<boost::filesystem::path,std::vector<std::string> >
    commandAndArgs(const std::string& command) const override;

    bool isDynamicallyAllocated() const override;

    void save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const override;

    ConfigPtr clone() const override;

    bool isDynamicallyCreatable() const override;
    bool isDynamicallyDestructable() const override;

  };

protected:
  Config serverConfig_;

  void runRsync
  (
      const std::vector<std::string>& args,
      std::function<void(int,const std::string&)> pf
  );


public:
  WSLLinuxServer(const Config& serverConfig);

  const RemoteServer::Config& config() const override;
  const Config& WSLServerConfig() const;

//  int executeCommand(const std::string& command, bool throwOnFail) override;
  static boost::filesystem::path WSLcommand();

  struct BackgroundJob : public RemoteServer::BackgroundJob
  {
  protected:
    std::unique_ptr<boost::process::child> process_;
  public:
    BackgroundJob(RemoteServer& server, std::unique_ptr<boost::process::child> process);
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


  static std::string defaultRepositoryURL(const ToolkitVersion& tv = ToolkitVersion::current() );
  static std::string installationPackageName(const ToolkitVersion& tv = ToolkitVersion::current() );
  static std::string defaultWSLDistributionName(const ToolkitVersion& tv = ToolkitVersion::current() );

  static std::vector<std::string> listWSLDistributions();

  ToolkitVersion checkInstalledVersion();
  void updateInstallation(
          std::function<void(const std::string&)> logCallback = [](const std::string&){}
          );

};


} // namespace insight

#endif // INSIGHT_WSLLINUXSERVER_H
