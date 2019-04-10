#include "remoteexecution.h"

#include <cstdlib>
#include "base/exception.h"
#include "base/tools.h"
#include "openfoam/openfoamcase.h"
#include "pstreams/pstream.h"

#include <regex>

using namespace std;
using namespace boost;

namespace insight
{

bool TaskSpoolerInterface::JobList::hasRunningJobs() const
{
  for (const auto& j: *this)
  {
    if (j.state==Running) return true;
  }
  return false;
}


bool TaskSpoolerInterface::JobList::hasQueuedJobs() const
{
  for (const auto& j: *this)
  {
    if (j.state==Queued) return true;
  }
  return false;
}


TaskSpoolerInterface::TaskSpoolerInterface(const boost::filesystem::path& socket)
  : socket_(socket),
    env_(boost::this_process::environment())
{
  env_["TS_SOCKET"]=socket.string();
}

TaskSpoolerInterface::JobList TaskSpoolerInterface::jobs() const
{
  JobList jl;
  boost::process::ipstream is;
  boost::process::child c("tsp", env_, boost::process::std_out > is);

  if (!c.running())
    throw insight::Exception("Could not execute task spooler executable!");

  std::vector<std::string> data;
  std::string line;
  int i=0;
  boost::regex re("^([^ ]*) +([^ ]*) +([^ ]*) +(.*)$");
  while (std::getline(is, line))
  {
    std::cout<<line<<std::endl;
    if (i>0)
    {
      boost::smatch m;
      if (boost::regex_match(line, m, re))
      {
        Job j;

        j.id=boost::lexical_cast<int>(m[1]);

        if (m[2]=="running")
          j.state=Running;
        else if (m[2]=="queued")
          j.state=Queued;
        else if (m[2]=="finished")
          j.state=Finished;
        else
          j.state=Unknown;

        j.output=boost::filesystem::path(m[3]);

        j.remainder=m[4];

        jl.push_back(j);
      }
    }
    i++;
  }

  c.wait();

  return jl;
}


void TaskSpoolerInterface::cancelAllJobs()
{
  while ( boost::process::system(boost::process::search_path("tsp"),
                                 boost::process::args("-k"),
                                 env_/*, boost::process::std_out > boost::process::null*/) == 0 )
  {
    boost::process::system(boost::process::search_path("tsp"),
                           boost::process::args("-C"),
                           env_);
  }
}



RemoteServerList::RemoteServerList()
{
  SharedPathList paths;
  for ( const bfs_path& p: paths )
  {
      if ( exists(p) && is_directory ( p ) )
      {
          bfs_path serverlist = bfs_path(p) / "remoteservers.list";

          if ( exists(serverlist) )
          {
              std::string line;
              std::ifstream f(serverlist.c_str());
              bool anything_read=false;
              int i=0;
              while (getline(f, line))
                {
                  i++;

                  std::regex pat("([^ ]+) *= *([^ ]+):([^ ]+)");
                  std::smatch m;
                  std::regex_match(line, m, pat);
                  if (m.size()!=4)
                    {
                      insight::Warning(boost::str(
                               boost::format("invalid remote server config in line %d of file %s (content: \"%s\"). Ignored")
                                                 % i % serverlist.string() % line
                                                 ));
                    }
                  else
                    {
                      RemoteServerInfo s;
                      std::string key= m[1];
                      s.serverName_ = m[2];
                      s.defaultDir_ = bfs_path(m[3]);
                      (*this)[key]=s;
                      anything_read=true;
                    }
                }

              if (!anything_read)
                insight::Warning("Could not read valid data from "+serverlist.string());
            }
        }
    }
}


const RemoteServerList::value_type RemoteServerList::findServer(const std::string& server) const
{
  auto i = find(server);
  if (i==end())
    {
      throw insight::Exception("Remote server \""+server+"\" not found in configuration!");
    }
  return *i;
}


RemoteServerList remoteServers;


void RemoteExecutionConfig::execRemoteCmd(const std::string& command)
{
    std::ostringstream cmd;

    cmd << "ssh " << server_ << " \"";
     cmd << "export TS_SOCKET="<<(remoteDir_/"tsp.socket")<<";";

     try {
         const OFEnvironment& cofe = OFEs::getCurrent();
         cmd << "source " << cofe.bashrc().filename() << ";";
     } catch (insight::Exception e) {
         // ignore, don't load OF config remotely
     }

     cmd << "cd "<<remoteDir_<<" && (";
     cmd << command;
    cmd << ")\"";

    std::cout<<cmd.str()<<std::endl;
    if (! ( std::system(cmd.str().c_str()) == 0 ))
    {
        throw insight::Exception("Could not execute command on server "+server_+": \""+cmd.str()+"\"");
    }
}

bool RemoteExecutionConfig::isValid() const
{
    return (!server_.empty())&&(!remoteDir_.empty());
}

RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig, const bfs_path& meta_file)
  : localDir_(location)
{
  if (meta_file.empty()) {
      meta_file_ = location/"meta.foam";
  } else {
      meta_file_ = meta_file;
  }

  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+location.string()+" from file "+meta_file_.string());

  if (!boost::filesystem::exists(meta_file_))
  {
      if (needConfig)
          throw insight::Exception("There is no remote execution configuration file present!");
  }
  else {
    std::ifstream f(meta_file_.c_str());
    std::string line;
    if (!getline(f, line))
      throw insight::Exception("Could not read first line from file "+meta_file_.string());

    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if (pair.size()!=2)
      throw insight::Exception("Error reading "+meta_file_.string()+": expected <server>:<remote directory>, got "+line);

    server_=pair[0];
    remoteDir_=pair[1];

    std::cout<<"configured "<<server_<<":"<<remoteDir_<<std::endl;
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

const boost::filesystem::path& RemoteExecutionConfig::metaFile() const
{
  return meta_file_;
}


std::vector<bfs_path> RemoteExecutionConfig::remoteLS() const
{
  std::vector<bfs_path> res;

  redi::ipstream p_in;

  p_in.open("ssh", { "ssh", server(), "ls", remoteDir().string() } );

  if (!p_in.is_open())
  {
    throw insight::Exception("RemoteExecutionConfig::remoteLS: Failed to launch directory listing subprocess!");
  }

  std::string line;
  while (std::getline(p_in.out(), line))
  {
    cout<<line<<endl;
    res.push_back(line);
  }
  while (std::getline(p_in.err(), line))
  {
    cerr<<"ERR: "<<line<<endl;
  }
  p_in.close();

  if (p_in.rdbuf()->status()!=0)
  {
    throw insight::Exception("RemoteExecutionConfig::remoteLS: command failed with nonzero return code.");
  }

  return res;
}


void RemoteExecutionConfig::syncToRemote(const std::vector<std::string>& exclude_pattern)
{
    std::ostringstream cmd;

    std::string excludes;
    excludes+="--exclude 'processor*' --exclude '*.foam' --exclude 'postProcessing' --exclude '*.socket' --exclude 'backup' --exclude 'archive' --exclude 'mnt_remote'";

    for (const auto& ex: exclude_pattern)
    {
      excludes+=" --exclude '"+ex+"'";
    }

    cmd << "rsync -avz --delete "+excludes+" . \""<<server_<<":"<<remoteDir_.string()<<"\"";

    std::system(cmd.str().c_str());
}

void RemoteExecutionConfig::syncToLocal(bool skipTimeSteps, const std::vector<std::string>& exclude_pattern)
{
    std::ostringstream cmd;

    cmd << "rsync -avz ";

    std::string excludes = "--exclude 'processor*' --exclude '*.foam' --exclude '*.socket' --exclude 'backup' --exclude 'archive' --exclude 'mnt_remote'";

    if (skipTimeSteps)
      {
        auto files = remoteLS();

        // remove non-numbers
        files.erase(remove_if(files.begin(), files.end(),
                [&](const bfs_path& f)
                {
                  try { lexical_cast<double>(f.c_str()); return false; }
                  catch (...) { return true; }
                }), files.end());

        for (const auto& f: files)
          {
            cmd<<" --exclude '"<<f.c_str()<<"'";
          }
      }

    for (const auto& ex: exclude_pattern)
    {
      excludes+=" --exclude '"+ex+"'";
    }

    cmd<<excludes<<" \""<<server_<<":"<<remoteDir_.string()<<"/*\" .";

    std::system(cmd.str().c_str());
}

void RemoteExecutionConfig::queueRemoteCommand(const std::string& command, bool waitForPreviousFinished)
{
  if (waitForPreviousFinished)
      execRemoteCmd("tsp -d " + command);
  else
      execRemoteCmd("tsp " + command);
}


void RemoteExecutionConfig::waitRemoteQueueFinished()
{
    execRemoteCmd("while tsp -c; do tsp -C; done");
}

void RemoteExecutionConfig::waitLastCommandFinished()
{
    execRemoteCmd("tsp -t");
}

void RemoteExecutionConfig::cancelRemoteCommands()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
}

void RemoteExecutionConfig::removeRemoteDir()
{
    execRemoteCmd("tsp -C; tsp -k; tsp -K");
    execRemoteCmd("rm -r *; cd ..; rmdir "+remoteDir_.string());
}

}
