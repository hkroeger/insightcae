#include "linuxremoteserver.h"
#include "base/boost_include.h"
#include "base/tools.h"
#include "boost/regex/v4/regex_fwd.hpp"
#include "base/cppextensions.h"
#include <cstdio>
#include "base/warningdispatcher.h"


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
    auto r = runCommand("LC_ALL=C mpstat -P all 1 1");

    if (r.exitCode==0)
    {
        std::istringstream out(r.out);
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
    if (!closed_)
    {
        try
        {
            close();
        }
        catch (const std::exception& e)
        {
            insight::Warning(
                "error while closing remote file %s: %s",
                remoteFilePath_.string().c_str(), e.what() );
        }
    }
}

std::ostream &LinuxRemoteServer::SSHRemoteStream::stream()
{
    return s_;
}

void LinuxRemoteServer::SSHRemoteStream::close()
{
    if (closed_) return;
    closed_=true;

    s_<<std::flush;
    s_.pipe().close();
    child_->wait();

    int ret = child_->exit_code();
    if (ret!=0)
    {
        throw insight::Exception(
            "writing remote file %s failed (exit code %d)",
            remoteFilePath_.string().c_str(), ret );
    }
}


std::unique_ptr<RemoteServer::RemoteStream> LinuxRemoteServer::remoteOFStream(
    const boost::filesystem::path &remoteFilePath,
    int totalBytes,
    std::function<void (int, const std::string &)> progress_callback
    )
{
    auto rs=std::make_unique<SSHRemoteStream>();
    rs->remoteFilePath_=remoteFilePath;

    rs->child_ = launchCommand(
        "cat > "+shellQuote(toUnixPath(remoteFilePath)),
            boost::process::std_in < rs->s_ );

    insight::assertion(
        rs->child_->running(),
        "remote cat process not running" );

    return rs;
}


/**
 * format the result of a failed remote command for an error message
 */
static std::string failureDescription(const RemoteServer::Config::CommandResult& r)
{
    std::string err = r.err;
    boost::trim(err);
    return str(boost::format("exit code %d%s")
               % r.exitCode
               % (err.empty() ? std::string() : ": "+err) );
}


/**
 * shell command, which succeeds, if a non-zombie process of the group exists
 * (zombies remain in the group without a reaping init process, e.g. in some containers)
 */
static std::string processGroupAliveCommand(int pgid)
{
  return "ps -e -o pgid=,stat= | awk '$1=="+std::to_string(pgid)+" && $2 !~ /^Z/ {f=1} END {exit !f}'";
}


LinuxRemoteServer::ProcessGroupJob::ProcessGroupJob(RemoteServer& server, int pgid)
  : RemoteServer::BackgroundJob(server),
    pgid_(pgid)
{}


void LinuxRemoteServer::ProcessGroupJob::kill()
{
  // terminate the whole group, escalate to SIGKILL after 5 s.
  // A group, which does not exist (anymore), is not an error.
  auto g = std::to_string(pgid_);
  std::string alive = processGroupAliveCommand(pgid_);
  server_.executeCommand(
        "kill -TERM -- -"+g+" 2>/dev/null || exit 0; "
        "for i in $(seq 1 50); do "+alive+" || exit 0; sleep 0.1; done; "
        "kill -KILL -- -"+g+" 2>/dev/null; exit 0",
        true );
}


bool LinuxRemoteServer::ProcessGroupJob::isRunning()
{
  return server_.executeCommand( processGroupAliveCommand(pgid_), false ) == 0;
}


RemoteServer::BackgroundJobPtr LinuxRemoteServer::launchBackgroundProcess(
        const std::string &cmd,
        const std::vector<ExpectedOutput>& eobd )
{
  // new session: the shell started by setsid is the leader,
  // its PID is the process group id of the job and all its children.
  // If no output is expected, the job releases the connection right after
  // reporting its PID (otherwise it would depend on it, e.g. on the ssh session).
  std::string job = "echo PID===$$===PID; ";
  if (eobd.empty())
      job += "exec </dev/null >/dev/null 2>&1; ";
  job += cmd;

  auto is = std::make_shared<boost::process::ipstream>();

  // stderr is captured as well: the launching process must not inherit our stderr
  // (it may live as long as the job) and error messages (e.g. from ssh) end up
  // in the exception message of lookForPattern
  auto process = launchCommand(
        "setsid bash -c "+shellQuote(job)+" </dev/null &",
        (boost::process::std_out & boost::process::std_err) > *is,
        boost::process::std_in < boost::process::null );

  std::vector<std::string> pidMatch;
  std::vector<ExpectedOutput> pats(eobd.begin(), eobd.end());
  pats.push_back( { boost::regex("PID===([0-9]+)===PID"), &pidMatch } );
  lookForPattern(*is, pats);

  insight::assertion(
      pidMatch.size()==2,
      "could not determine the PID of the remote background process" );
  int pgid=boost::lexical_cast<int>(pidMatch[1]);

  insight::dbg()<<"remote process group = "<<pgid<<std::endl;

  process->detach();

  return std::make_shared<ProcessGroupJob>(*this, pgid);
}


bool LinuxRemoteServer::checkIfDirectoryExists(const boost::filesystem::path& dir)
{
  int ret = executeCommand(
        "test -d "+shellQuote(toUnixPath(dir)), false );

  return ret==0;
}

boost::filesystem::path LinuxRemoteServer::getTemporaryDirectoryName(const boost::filesystem::path& templatePath)
{
  assertRunning();

  // create the directory (not only the name), so that it is reserved
  auto r = config().runCommand(
        "mkdir -p "+shellQuote(toUnixPath(templatePath.parent_path()))+
        " && mktemp -d "+shellQuote(toUnixPath(templatePath)) );

  if (r.exitCode!=0)
  {
    throw insight::Exception(
          "Could not create temporary remote directory from template %s (%s)",
          templatePath.string().c_str(), failureDescription(r).c_str() );
  }

  std::string dir = r.out;
  boost::trim(dir);
  insight::assertion(
        !dir.empty(),
        "no temporary directory name returned by remote server" );

  insight::dbg()<<dir<<std::endl;
  return dir;
}

void LinuxRemoteServer::createDirectory(const boost::filesystem::path& remoteDirectory)
{
  auto r = config().runCommand(
        "mkdir -p "+shellQuote(toUnixPath(remoteDirectory)) );

  if (r.exitCode!=0)
  {
    throw insight::Exception(
          "Failed to create remote directory %s (%s)",
          remoteDirectory.string().c_str(), failureDescription(r).c_str() );
  }
}

void LinuxRemoteServer::removeDirectory(const boost::filesystem::path& remoteDirectory)
{
  auto p = toUnixPath(remoteDirectory);
  boost::trim_right_if(p, boost::is_any_of("/"));
  insight::assertion(
        !p.empty(),
        "refusing to remove remote directory \"%s\"",
        remoteDirectory.string().c_str() );

  auto r = config().runCommand( "rm -rf "+shellQuote(p) );

  if (r.exitCode!=0)
  {
    throw insight::Exception(
          "Failed to remove remote directory %s (%s)",
          remoteDirectory.string().c_str(), failureDescription(r).c_str() );
  }
}

std::vector<boost::filesystem::path> LinuxRemoteServer::listRemoteDirectory(const boost::filesystem::path& remoteDirectory)
{
  auto r = config().runCommand(
        "ls -1 -- "+shellQuote(toUnixPath(remoteDirectory)) );

  if (r.exitCode!=0)
  {
    throw insight::Exception(
          "Could not list remote directory %s (%s)",
          remoteDirectory.string().c_str(), failureDescription(r).c_str() );
  }

  std::vector<bfs_path> res;
  std::istringstream is(r.out);
  std::string line;
  while (std::getline(is, line))
  {
    if (!line.empty())
      res.push_back(line);
  }
  return res;
}

std::vector<boost::filesystem::path> LinuxRemoteServer::listRemoteSubdirectories(const boost::filesystem::path& remoteDirectory)
{
  auto r = config().runCommand(
        "find "+shellQuote(toUnixPath(remoteDirectory)+"/") // add slash for symbolic links
        +" -mindepth 1 -maxdepth 1 -type d -printf '%P\\n'" );

  if (r.exitCode!=0)
  {
    throw insight::Exception(
          "Could not list subdirectories of remote directory %s (%s)",
          remoteDirectory.string().c_str(), failureDescription(r).c_str() );
  }

  std::vector<bfs_path> res;
  std::istringstream is(r.out);
  std::string line;
  while (std::getline(is, line))
  {
    if (!line.empty())
      res.push_back(line);
  }
  return res;
}



int LinuxRemoteServer::findFreeRemotePort() const
{
    auto r = config().runCommand("isPVFindPort.sh");

    if (r.exitCode!=0)
    {
      throw insight::Exception(
            "Failed to query remote server for free network port! (%s)",
            failureDescription(r).c_str() );
    }

    std::string outline;
    std::istringstream out(r.out);
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
