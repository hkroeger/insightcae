#include "remoteexecution.h"

#include <cstdlib>

#include "base/exception.h"
#include "base/tools.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/ofes.h"

#include <boost/asio.hpp>
#include <boost/process/async.hpp>

#include "pstreams/pstream.h"

#include <regex>

using namespace std;
using namespace boost;

namespace insight
{


void RemoteLocation::runRsync
(
    const std::vector<std::string>& args,
    std::function<void(int,const std::string&)> pf
)
{
  namespace bp=boost::process;
  bp::ipstream is;
  bp::child c
      (
       bp::search_path("rsync"),
       bp::args( args ),
       bp::std_out > is
      );

  std::string line;
  boost::regex pattern(".* ([^ ]*)% *([^ ]*) *([^ ]*) \\(xfr#([0-9]+), to-chk=([0-9]+)/([0-9]+)\\)");
  while (c.running() && std::getline(is, line) && !line.empty())
  {
    boost::smatch match;
    if (boost::regex_search( line, match, pattern, boost::match_default ))
    {
      std::string percent=match[1];
      std::string rate=match[2];
      std::string eta=match[3];
//        int i_file=to_number<int>(match[4]);
      int i_to_chk=to_number<int>(match[5]);
      int total_to_chk=to_number<int>(match[6]);

      double progress = total_to_chk==0? 1.0 : double(total_to_chk-i_to_chk) / double(total_to_chk);

      if (pf) pf(int(100.*progress), rate+", "+eta+" (current file: "+percent+")");
    }
  }

  c.wait();
}

RemoteLocation::RemoteLocation(const RemoteLocation& orec)
  : server_(orec.server_),
    remoteDir_(orec.remoteDir_)
{
}

RemoteLocation::RemoteLocation(const boost::filesystem::path& mf)
{
  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+mf.parent_path().string()+" from file "+mf.string());

  if (!boost::filesystem::exists(mf))
  {
    throw insight::Exception("There is no remote execution configuration file present!");
  }
  else
  {
    std::ifstream f(mf.c_str());
    std::string line;
    if (!getline(f, line))
      throw insight::Exception("Could not read first line from file "+mf.string());

    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if (pair.size()!=2)
      throw insight::Exception("Error reading "+mf.string()+": expected <server>:<remote directory>, got "+line);

    server_=pair[0];
    remoteDir_=pair[1];
  }
}


RemoteLocation::RemoteLocation(const std::string& serverName, const boost::filesystem::path& remoteDir)
  : server_(serverName),
    remoteDir_(remoteDir)
{
}


const std::string& RemoteLocation::server() const
{
  return server_;
}



const boost::filesystem::path& RemoteLocation::remoteDir() const
{
  return remoteDir_;
}



std::vector<bfs_path> RemoteLocation::remoteLS() const
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

std::vector<bfs_path> RemoteLocation::remoteSubdirs() const
{
  std::vector<bfs_path> res;
  boost::process::ipstream is;
  std::shared_ptr<boost::process::child> c;

  c.reset(new boost::process::child(
            boost::process::search_path("ssh"),
            boost::process::args({server(),
                                  "find", remoteDir().string()+"/", // add slash for symbolic links
                                  "-maxdepth", "1", "-type", "d", "-printf", "%P\\\\n"}),
            boost::process::std_out > is
            ));

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


void RemoteLocation::putFile
(
    const boost::filesystem::path& localFile,
    const boost::filesystem::path& remoteFileName,
    std::function<void(int,const std::string&)> pf
    )
{
  boost::filesystem::path lf=localFile, rf=remoteFileName;
  //if (lf.is_relative()) lf=localDir_/lf;
  if (rf.is_relative()) rf=remoteDir_/rf;
  std::vector<std::string> args=
      {
       lf.string(),
       server_+":"+rf.string()
      };

  runRsync(args, pf);
}


void RemoteLocation::syncToRemote
(
    const boost::filesystem::path& localDir,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
    std::vector<std::string> args=
        {
         "-az",
         "--delete",
         "--info=progress",

         "--exclude", "processor*",
         "--exclude", "*.foam",
         "--exclude", "postProcessing",
         "--exclude", "*.socket",
         "--exclude", "backup",
         "--exclude", "archive",
         "--exclude", "mnt_remote"
        };

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    args.push_back(localDir.string()+"/");
    args.push_back(server_+":"+remoteDir_.string());

    runRsync(args, pf);
}

void RemoteLocation::syncToLocal
(
    const boost::filesystem::path& localDir,
    bool skipTimeSteps,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
    std::vector<std::string> args;

    args =
    {
      "-az",
      "--info=progress",
      "--exclude", "processor*",
      "--exclude", "*.foam",
      "--exclude", "*.socket",
      "--exclude", "backup",
      "--exclude", "archive",
      "--exclude", "mnt_remote"
    };


    if (skipTimeSteps)
      {
        auto files = remoteLS();

        // remove non-numbers
        files.erase(remove_if(files.begin(), files.end(),
                [&](const bfs_path& f)
                {
                  try { to_number<double>(f.c_str()); return false; }
                  catch (...) { return true; }
                }), files.end());

        for (const auto& f: files)
          {
            args.push_back("--exclude");
            args.push_back(f.c_str());
          }
      }

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    args.push_back(server_+":"+remoteDir_.string()+"/");
    args.push_back(localDir.string());

    runRsync(args, pf);
}


void RemoteLocation::removeRemoteDir()
{
  if (remoteDirExists())
  {
    int ret = boost::process::system(
                boost::process::search_path("ssh"),
                boost::process::args({
                  server(),
                  "rm -rf \""+remoteDir().string()+"\""
                })
          );
    if (ret!=0)
      throw insight::Exception("Failed to remote remote directory!");
  }
}



bool RemoteLocation::remoteDirExists() const
{
  if (server_.empty() || remoteDir_.empty())
    return false;

  int ret = boost::process::system(
              boost::process::search_path("ssh"),
              boost::process::args({server(), "cd", remoteDir().string()})
        );

  if (ret==0)
    return true;
  else
    return false;
}




// ====================================================================================
// ======== RemoteExecutionConfig



boost::filesystem::path RemoteExecutionConfig::socket() const
{
  return remoteDir()/"tsp.socket";
}

void RemoteExecutionConfig::execRemoteCmd(const std::string& command)
{
    std::ostringstream cmd;

    cmd << "ssh " << server_ << " \"";
     cmd << "export TS_SOCKET="<<socket()<<";";

     try
     {
         const OFEnvironment& cofe = OFEs::getCurrent();
         cmd << "source " << cofe.bashrc().filename() << ";";
     }
     catch (const std::exception& /*e*/) {
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











RemoteExecutionConfig::RemoteExecutionConfig(const RemoteExecutionConfig &o)
  : RemoteLocation(o),
    meta_file_(o.meta_file_),
    localDir_(o.localDir_)
{
}




RemoteExecutionConfig::RemoteExecutionConfig(const boost::filesystem::path& location, bool needConfig, const bfs_path& meta_file)
  : RemoteLocation( meta_file.empty() ? location/"meta.foam" : meta_file ),
    meta_file_( meta_file.empty() ? location/"meta.foam" : meta_file ),
    localDir_(location)
{
}


const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::metaFile() const
{
  return meta_file_;
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

void RemoteExecutionConfig::syncToRemote(
    const std::vector<string> &exclude_pattern,
    std::function<void (int, const string &)> progress_callback
    )
{
  RemoteLocation::syncToRemote(localDir_, exclude_pattern, progress_callback);
}

void RemoteExecutionConfig::syncToLocal(
    bool skipTimeSteps,
    const std::vector<string> &exclude_pattern,
    std::function<void (int, const string &)> progress_callback)
{
  RemoteLocation::syncToLocal(localDir_, skipTimeSteps, exclude_pattern, progress_callback);
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
  execRemoteCmd("tsp -K");
  RemoteLocation::removeRemoteDir();
}

bool RemoteExecutionConfig::isValid() const
{
    return (!server_.empty())&&(!remoteDir_.empty());
}

}
