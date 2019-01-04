#include "remoteexecution.h"

#include <cstdlib>
#include "base/exception.h"

namespace insight
{


RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location)
  : localDir_(location)
{
  boost::filesystem::path metafile = location/"meta.foam";

  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+location.string()+" from file "+metafile.string());

  if (!boost::filesystem::exists(metafile))
  {
    throw insight::Exception("There is no remote execution configuration file present!");
  }
  else {
    std::ifstream f(metafile.c_str());
    std::string line;
    if (!getline(f, line))
      throw insight::Exception("Could not read first line from file "+metafile.string());

    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if (pair.size()!=2)
      throw insight::Exception("Error reading "+metafile.string()+": expected <server>:<remote directory>, got "+line);

    server_=pair[0];
    remoteDir_=pair[1];
  }
}

const std::string& RemoteExecutionConfig::server() const
{
  return server_;
}

const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::remoteDir() const
{
  return remoteDir_;
}


void RemoteExecutionConfig::syncToRemote()
{
}

void RemoteExecutionConfig::syncToLocal()
{
}

void RemoteExecutionConfig::executeRemote(const std::string& command)
{
  std::ostringstream cmd;
  cmd << "ssh " << server_;
  if (! ( std::system(cmd.str().c_str()) == 0 ))
  {
      throw insight::Exception("Could not execute command on server "+server_+": \""+cmd.str()+"\"");
  }
}

}
