#ifndef INSIGHT_LINUXREMOTESERVER_H
#define INSIGHT_LINUXREMOTESERVER_H

#include "remoteserver.h"
#include "boost/process/detail/child_decl.hpp"
#include "boost/process/pipe.hpp"
#include <functional>
#include <memory>

namespace insight {

std::string toUnixPath(const boost::filesystem::path& wp);

class LinuxRemoteServer
    : public RemoteServer
{
public:
    struct Config : public RemoteServer::Config
    {
        Config(const boost::filesystem::path& bp, int np);
        bool isRunning() const override;
        int occupiedProcessors(int* nProcAvail=nullptr) const override;
    };

    struct SSHRemoteStream : public RemoteStream {
        std::unique_ptr<boost::process::child> child_;
        boost::process::opstream s_;

        ~SSHRemoteStream();
        std::ostream& stream() override;
    };

    std::unique_ptr<RemoteStream> remoteOFStream
        (
            const boost::filesystem::path& remoteFilePath,
            int totalBytes,
            std::function<void(int progress,const std::string& status_text)> progress_callback =
            std::function<void(int,const std::string&)>()
            ) override;

  bool checkIfDirectoryExists(const boost::filesystem::path& dir) override;
  boost::filesystem::path getTemporaryDirectoryName(const boost::filesystem::path& templatePath) override;
  void createDirectory(const boost::filesystem::path& remoteDirectory) override;
  void removeDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory) override;

  int findFreeRemotePort() const override;
};

} // namespace insight

#endif // INSIGHT_LINUXREMOTESERVER_H
