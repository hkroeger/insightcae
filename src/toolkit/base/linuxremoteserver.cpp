#include "linuxremoteserver.h"

using namespace std;


namespace insight {


bool LinuxRemoteServer::checkIfDirectoryExists(const boost::filesystem::path& dir)
{
  int ret = executeCommand(
        "cd \""+dir.string()+"\"", false );

  if (ret==0)
    return true;
  else
    return false;
}

boost::filesystem::path LinuxRemoteServer::createTemporaryDirectory(const boost::filesystem::path& templatePath)
{
  boost::process::ipstream out;

  int ret = executeCommand(
        "mktemp -d \""+templatePath.string()+"\"", false,
        boost::process::std_out > out );

  if (ret==0)
  {
    string line;
    getline(out, line);
    return line;
  }
  else
  {
    throw insight::Exception("Could not auto-create remote directory!");
  }
}

void LinuxRemoteServer::createDirectory(const boost::filesystem::path& remoteDirectory)
{
  int ret = executeCommand(
        "mkdir -p \""+remoteDirectory.string()+"\"", false );

  if (ret==0)
  {
    throw insight::Exception("Failed to create remote directory!");
  }
}

void LinuxRemoteServer::removeDirectory(const boost::filesystem::path& remoteDirectory)
{
  int ret = executeCommand(
        "rm -rf \""+remoteDirectory.string()+"\"", false );

  if (ret==0)
  {
    throw insight::Exception("Failed to remove remote directory!");
  }
}

std::vector<boost::filesystem::path> LinuxRemoteServer::listRemoteDirectory(const boost::filesystem::path& remoteDirectory)
{
  std::vector<bfs_path> res;

  boost::process::ipstream is, ise;
  auto childPtr = launchCommand(
        "ls \""+remoteDirectory.string()+"\"",
        boost::process::std_out > is, boost::process::std_err > ise );
  if (!childPtr->running())
    throw insight::Exception("RemoteExecutionConfig::remoteLS: Failed to launch directory listing subprocess!");

  std::string line;
  while (std::getline(is, line))
  {
    cout<<line<<endl;
    res.push_back(line);
  }
  while (std::getline(ise, line))
  {
    cerr<<"ERR: "<<line<<endl;
  }

  childPtr->wait();

  return res;
}

std::vector<boost::filesystem::path> LinuxRemoteServer::listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory)
{
  std::vector<bfs_path> res;
  boost::process::ipstream is;

  auto c = launchCommand(
        "find", remoteDirectory.string()+"/" // add slash for symbolic links
        " -maxdepth 1 -type d -printf \"%P\\\\n\"",
        boost::process::std_out > is );

  if (!c->running())
    throw insight::Exception("Could not execute remote dir list process!");

  std::string line;
  while (std::getline(is, line))
  {
    res.push_back(line);
  }

  c->wait();

  return res;
}



bool LinuxRemoteServer::hostIsAvailable()
{
  return (executeCommand("exit", false) == 0);
}


} // namespace insight
