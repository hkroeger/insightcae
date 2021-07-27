#include "linuxremoteserver.h"

using namespace std;


namespace insight {

std::string toUnixPath(const boost::filesystem::path& wp)
{
  auto wpl = wp.relative_path().generic_path().string();
  if (wp.is_absolute() || ( (wpl.size()>0) && (wp.string()[0]=='/' || wp.string()[0]=='\\')) )
    wpl="/"+wpl;
  return wpl;
}

bool LinuxRemoteServer::checkIfDirectoryExists(const boost::filesystem::path& dir)
{
  int ret = executeCommand(
        "cd \""+toUnixPath(dir)+"\"", false );

  if (ret==0)
    return true;
  else
    return false;
}

boost::filesystem::path LinuxRemoteServer::getTemporaryDirectoryName(const boost::filesystem::path& templatePath)
{
  boost::process::ipstream out;

  int ret = executeCommand(
        "mktemp -du \""+toUnixPath(templatePath)+"\"", false,
        boost::process::std_out > out,
        boost::process::std_err > stderr,
        boost::process::std_in < boost::process::null
        );

  if (ret==0)
  {
    string line;
    getline(out, line);
    insight::dbg()<<line<<std::endl;
    return line;
  }
  else
  {
    throw insight::Exception("Could not find temporary remote directory name!");
  }

  return "";
}

void LinuxRemoteServer::createDirectory(const boost::filesystem::path& remoteDirectory)
{
  int ret = executeCommand(
        "mkdir -p \""+toUnixPath(remoteDirectory)+"\"", false );

  if (ret!=0)
  {
    throw insight::Exception("Failed to create remote directory!");
  }
}

void LinuxRemoteServer::removeDirectory(const boost::filesystem::path& remoteDirectory)
{
  int ret = executeCommand(
        "rm -rf \""+toUnixPath(remoteDirectory)+"\"", false );

  if (ret!=0)
  {
    throw insight::Exception("Failed to remove remote directory!");
  }
}

std::vector<boost::filesystem::path> LinuxRemoteServer::listRemoteDirectory(const boost::filesystem::path& remoteDirectory)
{
  std::vector<bfs_path> res;

  boost::process::ipstream is, ise;
  auto childPtr = launchCommand(
        "ls \""+toUnixPath(remoteDirectory)+"\"",
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
        "find", toUnixPath(remoteDirectory)+"/" // add slash for symbolic links
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
  setRunning(executeCommand("exit", false) == 0);
  return isRunning();
}


} // namespace insight
