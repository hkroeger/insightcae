#include "linuxremoteserver.h"
#include "base/tools.h"
#include "boost/regex/v4/regex_fwd.hpp"
#include "base/cppextensions.h"
#include <cstdio>


using namespace std;


namespace insight {


LinuxRemoteServer::Config::Config(const boost::filesystem::path &bp, int np)
    : RemoteServer::Config(bp, np)
{}

bool LinuxRemoteServer::Config::isRunning() const
{
    return executeCommand("exit") == 0;
}

int LinuxRemoteServer::Config::occupiedProcessors(int* nProcAvail) const
{
    boost::process::ipstream out;

    int ret = executeCommand(
        "LC_ALL=C mpstat -P all 1 1",
        boost::process::std_out > out,
        boost::process::std_err > stderr,
        boost::process::std_in < boost::process::null
        );

    if (ret==0)
    {
        string line1, line4;

        getline(out, line1);
        getline(out, line4); //discard
        getline(out, line4); //discard
        getline(out, line4);

        boost::regex re1("(.*) \\((.*)\\) *(.*) *_(.*)_	\\(([0-9]*) CPU\\)");
        boost::regex re4(".* ([^ ]*)$");

        boost::smatch m1, m4;
        insight::assertion(
            boost::regex_match(line1, m1, re1),
            "unexpected output line 1 from mpstat on %s", c_str());
        insight::assertion(
            boost::regex_match(line4, m4, re4),
            "unexpected output line 4 from mpstat on %s", c_str());

        double frac=1.-insight::toNumber<double>(m4[1])/100.;
        insight::assertion(
            (frac>=0.) && (frac<=1.),
            "expected to be utilization fraction between 0 and 1. Got %g", frac);

        int np=insight::toNumber<int>(m1[5]);
        if (nProcAvail) *nProcAvail=np;

        return std::ceil(frac*double(np));
    }
    else
    {
        insight::Warning("Could not execute mpstat on %s!", c_str());
    }

    return np_;
}




std::string toUnixPath(const boost::filesystem::path& wp)
{
  auto wpl = wp.relative_path().generic_path().string();
  if (wp.is_absolute() || ( (wpl.size()>0) && (wp.string()[0]=='/' || wp.string()[0]=='\\')) )
    wpl="/"+wpl;
  return wpl;
}


LinuxRemoteServer::SSHRemoteStream::~SSHRemoteStream()
{
    s_<<std::flush;
    s_.pipe().close();
    child_->wait();
}

std::ostream &LinuxRemoteServer::SSHRemoteStream::stream()
{
    return s_;
}


std::unique_ptr<RemoteServer::RemoteStream> LinuxRemoteServer::remoteOFStream(
    const boost::filesystem::path &remoteFilePath,
    int totalBytes,
    std::function<void (int, const std::string &)> progress_callback
    )
{
    auto rs=std::make_unique<SSHRemoteStream>();

    rs->child_ = launchCommand(
        "cat - > \""+toUnixPath(remoteFilePath)+"\"",
            boost::process::std_in < rs->s_ );

    insight::assertion(
        rs->child_->running(),
        "remote cat process not running" );

    return rs;
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
    assertRunning();
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
        boost::process::std_out > is,
        boost::process::std_err > ise,
        boost::process::std_in < boost::process::null
        );
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
        "find "+toUnixPath(remoteDirectory)+"/" // add slash for symbolic links
        " -maxdepth 1 -type d -printf \"%P\\\\n\"",
        boost::process::std_out > is,
        boost::process::std_err > stderr,
        boost::process::std_in < boost::process::null
        );

  if (!c->running())
    throw insight::Exception("Could not execute remote dir list process!");

  std::string line;
  while (std::getline(is, line))
  {
    if (!line.empty())
      res.push_back(line);
  }

  c->wait();

  return res;
}



int LinuxRemoteServer::findFreeRemotePort() const
{
    boost::process::ipstream out;

    int ret = executeCommand(
                "isPVFindPort.sh", false,
                boost::process::std_out > out,
                boost::process::std_err > stderr,
                boost::process::std_in < boost::process::null
                );

    if (ret!=0)
    {
      throw insight::Exception(
            str( boost::format("Failed to query remote server for free network port!") )
            );
    }

    std::string outline;
    getline(out, outline);
    std::vector<std::string> parts;
    boost::split(parts, outline, boost::is_any_of(" "));
    if (parts.size()==2)
    {
      if (parts[0]=="PORT")
        return toNumber<int>(parts[1]);
    }

    throw insight::Exception("unexpected answer: \""+outline+"\"");
}





} // namespace insight
