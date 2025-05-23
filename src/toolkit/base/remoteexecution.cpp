#include "remoteexecution.h"

#include <cstdlib>

#include "base/exception.h"
#include "base/tools.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"

#include <boost/asio.hpp>
#include <boost/process/async.hpp>

#include <regex>
#include "rapidxml/rapidxml_print.hpp"

#include <signal.h>




using namespace std;
using namespace boost;
namespace bp = boost::process;




namespace insight
{




boost::filesystem::path RemoteLocation::socket() const
{
  return (remoteDir()/"tsp.socket").generic_path();
}




void RemoteLocation::queueRemoteCommand(const std::string& command, bool waitForPreviousFinished)
{
  if (waitForPreviousFinished)
      execRemoteCmd("tsp -d " + command);
  else
      execRemoteCmd("tsp " + command);
}




void RemoteLocation::waitRemoteQueueFinished()
{
    execRemoteCmd("while tsp -c; do tsp -C; done");
}




void RemoteLocation::waitLastCommandFinished()
{
    execRemoteCmd("tsp -t");
}




void RemoteLocation::cancelRemoteCommands()
{
  execRemoteCmd("tsp -C; tsp -k; tsp -K");
}





RemoteExecutionConfig::RemoteExecutionConfig(const RemoteExecutionConfig &o)
  : RemoteLocation(o),
    localREConfigFile_(o.localREConfigFile_),
    localDir_(o.localDir_)
{
}


RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location,
                                             const bfs_path& localREConfigFile)
  : RemoteLocation( localREConfigFile.empty() ? defaultConfigFile(location) : localREConfigFile ),
    localREConfigFile_( localREConfigFile.empty() ? defaultConfigFile(location) : localREConfigFile ),
    localDir_(location)
{
}


RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location,
                                             const RemoteLocation& rloc,
                                             const bfs_path& localREConfigFile)
  : RemoteLocation(rloc),
    localREConfigFile_( localREConfigFile.empty() ? defaultConfigFile(location) : localREConfigFile ),
    localDir_(location)
{
  writeConfig(localREConfigFile);
}



RemoteExecutionConfig::RemoteExecutionConfig(RemoteServer::ConfigPtr rsc,
                                             const filesystem::path &location,
                                             const filesystem::path &remotePath,
                                             const filesystem::path &localREConfigFile)
  : RemoteLocation(rsc, remotePath),
    localDir_(location)
{
  writeConfig(localREConfigFile);
}


void RemoteExecutionConfig::writeConfig(const boost::filesystem::path& localREConfigFile) const
{
  writeConfigFile(
        localREConfigFile.empty() ? defaultConfigFile(localDir()) : localREConfigFile
  );
}





const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}




const boost::filesystem::path& RemoteExecutionConfig::metaFile() const
{
  return localREConfigFile_;
}




void RemoteExecutionConfig::cleanup(bool forceRemoval)
{
  RemoteLocation::cleanup(forceRemoval);
  boost::filesystem::remove(metaFile());
}




void RemoteExecutionConfig::putFile
(
    const boost::filesystem::path& localFile,
    const boost::filesystem::path& remoteFileName,
    std::function<void(int,const std::string&)> pf
    )
{
  boost::filesystem::path lf=localFile;
  if (lf.is_relative()) lf=localDir_/lf;

  RemoteLocation::putFile(lf, remoteFileName, pf);
}




std::unique_ptr<RemoteServer::RemoteStream>
RemoteExecutionConfig::remoteOFStream(
    const boost::filesystem::path &relativeRemoteFilePath,
    int totalBytes,
    std::function<void (int, const std::string &)> progress_callback
)
{
    return server()->remoteOFStream(
        remoteDir()/relativeRemoteFilePath,
        totalBytes, progress_callback
        );
}




void RemoteExecutionConfig::syncToRemote(
    bool includeProcessorDirectories,
    const std::vector<string> &exclude_pattern,
    std::function<void (int, const string &)> progress_callback
    )
{
  RemoteLocation::syncToRemote(
              localDir_,
              includeProcessorDirectories,
              exclude_pattern, progress_callback );
}




void RemoteExecutionConfig::syncToLocal(
    bool includeProcessorDirectories,
    bool skipTimeSteps,
    const std::vector<string> &exclude_pattern,
    std::function<void (int, const string &)> progress_callback)
{
    RemoteLocation::syncToLocal(
                localDir_,
                includeProcessorDirectories,
                skipTimeSteps,
                exclude_pattern, progress_callback);
}




boost::filesystem::path RemoteExecutionConfig::defaultConfigFileName()
{
    return "meta.foam";
}




boost::filesystem::path RemoteExecutionConfig::defaultConfigFile(const boost::filesystem::path& location)
{
  return location/defaultConfigFileName();
}




}
