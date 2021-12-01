#ifndef INSIGHT_LINUXREMOTESERVER_H
#define INSIGHT_LINUXREMOTESERVER_H

#include "remoteserver.h"

namespace insight {

std::string toUnixPath(const boost::filesystem::path& wp);

class LinuxRemoteServer
    : public RemoteServer
{
public:

  bool checkIfDirectoryExists(const boost::filesystem::path& dir) override;
  boost::filesystem::path getTemporaryDirectoryName(const boost::filesystem::path& templatePath) override;
  void createDirectory(const boost::filesystem::path& remoteDirectory) override;
  void removeDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteDirectory(const boost::filesystem::path& remoteDirectory) override;
  std::vector<boost::filesystem::path> listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory) override;

  int findFreeRemotePort() const override;
  bool hostIsAvailable() override;
};

} // namespace insight

#endif // INSIGHT_LINUXREMOTESERVER_H
