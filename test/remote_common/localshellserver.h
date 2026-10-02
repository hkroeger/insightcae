#ifndef INSIGHT_LOCALSHELLSERVER_H
#define INSIGHT_LOCALSHELLSERVER_H

#include "base/linuxremoteserver.h"

namespace insight {
namespace remotetest {


/**
 * @brief The LocalShellServer class
 * Test-only "remote" server, which executes all commands through a local bash.
 *
 * It uses the same mechanisms as the real Linux backends
 * (command execution through a shell like WSLLinuxServer,
 * background jobs from LinuxRemoteServer),
 * so that the code shared in LinuxRemoteServer can be tested without
 * any SSH or WSL installation.
 *
 * File transfer is done by rsync, if available, otherwise by a plain copy.
 */
class LocalShellServer
    : public LinuxRemoteServer
{
public:
    struct Config : public LinuxRemoteServer::Config
    {
        Config(
            const boost::filesystem::path& baseDirectory,
            int np=1,
            const std::string& label="localshell" );

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

public:
    LocalShellServer(const Config& cfg);

    const RemoteServer::Config& config() const override;

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
};


} // namespace remotetest
} // namespace insight

#endif // INSIGHT_LOCALSHELLSERVER_H
