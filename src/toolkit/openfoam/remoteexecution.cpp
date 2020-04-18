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
#include "rapidxml/rapidxml_print.hpp"

#include <signal.h>

using namespace std;
using namespace boost;
namespace bp = boost::process;

namespace insight
{


bool hostAvailable(const string &host)
{
  int ret = bp::system(
        bp::search_path("ssh"),
        bp::args({ "-q", host, "exit" })
        );

  return (ret==0);
}


void RemoteLocation::runRsync
(
    const std::vector<std::string>& args,
    std::function<void(int,const std::string&)> pf
)
{
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
  : launchScript_(orec.launchScript_),
    server_(orec.server_),
    remoteDir_(orec.remoteDir_),
    autoCreateRemoteDirRequired_(orec.autoCreateRemoteDirRequired_),
    isValid_(orec.isValid_)
{}



RemoteLocation::RemoteLocation(const boost::filesystem::path& mf)
  : autoCreateRemoteDirRequired_(false)
{
  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+mf.parent_path().string()+" from file "+mf.string());

  if (!boost::filesystem::exists(mf))
  {
    throw insight::Exception("There is no remote execution configuration file present!");
  }
  else
  {
    std::string line;
    {
      std::ifstream f(mf.c_str());
      if (!getline(f, line))
        throw insight::Exception("Could not read line from file "+mf.string());
    }
    std::vector<std::string> pair;
    boost::split(pair, line, boost::is_any_of(":"));
    if ( (!algorithm::starts_with(line, "<")) && pair.size()==2)
    {
      server_=pair[0];
      remoteDir_=pair[1];
    }
    else
    {
      // read xml
      string content;
      try
      {
          std::ifstream in(mf.c_str());
          istreambuf_iterator<char> fbegin(in), fend;
          std::copy(fbegin, fend, back_inserter(content));
      }
      catch (...)
      {
          throw insight::Exception("Failed to read file "+mf.string());
      }

      using namespace rapidxml;
      xml_document<> doc;
      doc.parse<0>(&content[0]);

      auto *rootnode = doc.first_node();
      if (!rootnode || rootnode->name()!=string("remote") )
        throw insight::Exception("No valid \"remote\" node found in XML!");

      if (auto ls = rootnode->first_attribute("launchScript"))
        launchScript_ = ls->value();
      if (auto s = rootnode->first_attribute("server"))
        server_ = s->value();
      if (auto rd = rootnode->first_attribute("directory"))
        remoteDir_ = rd->value();
    }

    validate();

    if (!isValid_)
    {
      throw insight::Exception("Remote execution configuration is invalid!");
    }
  }
}


RemoteLocation::RemoteLocation(const RemoteServerInfo& rsi, const boost::filesystem::path& remotePath)
  : launchScript_( rsi.hasLaunchScript_ ? rsi.server_ : "" ),
    server_(rsi.hasLaunchScript_ ? "" : rsi.server_ ),
    remoteDir_( rsi.defaultDir_ ),
    autoCreateRemoteDirRequired_( remotePath.empty() )
{
  if (!remotePath.empty())
  {
    remoteDir_ = remotePath;
  }
  else
  {
    remoteDir_ = rsi.defaultDir_;
  }

  initialize();
}

RemoteLocation::~RemoteLocation()
{
  stopTunnels();
}

void RemoteLocation::createTunnels(
    std::vector<boost::tuple<int, string, int> > remoteListenPorts,
    std::vector<boost::tuple<int, string, int> > localListenPorts
    )
{
  std::vector<std::string> args = { "-N" };

  for (const auto& rlp: remoteListenPorts)
  {
    args.insert(
          std::end(args),
          {"-R", str(format("%d:%s:%d") % get<0>(rlp) % get<1>(rlp) % get<2>(rlp)) }
    );
  }

  for (const auto& llp: localListenPorts)
  {
    args.insert(
          std::end(args),
          {"-L", str(format("%d:%s:%d") % get<0>(llp) % get<1>(llp) % get<2>(llp)) }
    );
  }

  args.push_back( server() );

  tunnelProcesses_.push_back(
      bp::child
      (
       bp::search_path("ssh"),
       bp::args( args )
      )
        );
}

void RemoteLocation::stopTunnels()
{
  size_t i=0;
  for (auto& t: tunnelProcesses_)
  {
    i++;
    cout << "Stopping tunnel instance "<<i<<"/"<<tunnelProcesses_.size()<<"..." << endl;
    //try graceful exit
    kill(t.id(), SIGTERM);
    if (!t.wait_for( std::chrono::seconds(10) ))
    {
      t.terminate();
    }

  }
  tunnelProcesses_.clear();
}


const std::string& RemoteLocation::server() const
{
  return server_;
}



const boost::filesystem::path& RemoteLocation::remoteDir() const
{
  return remoteDir_;
}

void RemoteLocation::cleanup()
{
  CurrentExceptionContext ex("clean remote server");
  assertValid();

  execRemoteCmd("tsp -K");

  removeRemoteDir();
  disposeHost();
}




void RemoteLocation::launchHost()
{
  if (!launchScript_.empty())
  {
    CurrentExceptionContext ce("executing launch script");

    bp::ipstream out;

    int ret = bp::system(
          bp::search_path("sh"),
          bp::args({ "-c",
                     collectIntoSingleCommand(launchScript_.string(), {"launch"})
                   }),
          bp::std_out > out
          );

    if (ret!=0)
    {
      throw insight::Exception(
            str( format("Failed to execute launch script %s for remote host!") % launchScript_)
            );
    }
    else
    {
      getline(out, server_);
    }
  }

  if (!serverIsUp())
    throw insight::Exception("Could not establish connection to server "+server_+"!");
}




void RemoteLocation::initialize()
{
  if (!serverIsUp()) launchHost();

  if (autoCreateRemoteDirRequired_)
  {
    bp::ipstream out;

    int ret = bp::system(
          bp::search_path("ssh"),
          bp::args({ server(),
                     "mktemp", "-d", (remoteDir_/"irXXXXXX").string() }),
          bp::std_out > out
          );

    if (ret==0)
    {
      string line;
      getline(out, line);
      remoteDir_=line;
    }
    else
    {
      throw insight::Exception("Could not auto-create remote directory!");
    }
  }
  else if (!remoteDirExists())
  {
    int ret = bp::system(
                bp::search_path("ssh"),
                bp::args({
                  server(),
                  "mkdir -p \""+remoteDir().string()+"\""
                })
          );

    if (ret!=0)
      throw insight::Exception("Failed to create remote directory!");
  }

  validate();
}

void RemoteLocation::validate()
{
  isValid_=false;
  if (!server_.empty() && !remoteDir_.empty())
  {
    if (serverIsUp())
    {
      if (remoteDirExists())
        isValid_=true;
    }
  }
}


void RemoteLocation::removeRemoteDir()
{
  if (remoteDirExists())
  {
    int ret = bp::system(
                bp::search_path("ssh"),
                bp::args({
                  server(),
                  "rm -rf \""+remoteDir().string()+"\""
                })
          );
    if (ret!=0)
      throw insight::Exception("Failed to remove remote directory!");

    isValid_=false;
  }
}

void RemoteLocation::disposeHost()
{
  if (!launchScript_.empty())
  {
    CurrentExceptionContext ce("executing dispose script");

    int ret = bp::system(
          bp::search_path("sh"),
          bp::args({ "-c",
                     collectIntoSingleCommand(launchScript_.string(), {"dispose", server()})
                   })
          );

    if (ret!=0)
    {
      throw insight::Exception(
            str( format("Failed to remote remote directory!") % launchScript_ )
            );
    }

    isValid_=false;
  }
}

void RemoteLocation::assertValid() const
{
  if (!isValid_)
    throw insight::Exception("Error: attempted communication with invalid remote location!");
}

bool RemoteLocation::serverIsUp() const
{
  if (!server_.empty())
    return hostAvailable(server_);
  else
    return false;
}



std::vector<bfs_path> RemoteLocation::remoteLS() const
{
  CurrentExceptionContext ex("list remote directory contents");

  assertValid();

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
  CurrentExceptionContext ex("list remote subdirectories");
  assertValid();

  std::vector<bfs_path> res;
  bp::ipstream is;
  std::shared_ptr<bp::child> c;

  c.reset(new bp::child(
            bp::search_path("ssh"),
            bp::args({server(),
                                  "find", remoteDir().string()+"/", // add slash for symbolic links
                                  "-maxdepth", "1", "-type", "d", "-printf", "%P\\\\n"}),
            bp::std_out > is
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


int RemoteLocation::execRemoteCmd(const std::string& command, bool throwOnFail)
{
  insight::CurrentExceptionContext ex("executing command on remote host: "+command);
  assertValid();

    std::ostringstream cmd;

    cmd << "export TS_SOCKET="<<socket()<<";";

    try
    {
       const OFEnvironment& cofe = OFEs::getCurrent();
       cmd << "source " << cofe.bashrc().filename() << ";";
    }
    catch (const std::exception& e) {
      std::cerr<<e.what()<<std::endl;
       // ignore, don't load OF config remotely
    }

    cmd << "cd "<<remoteDir_<<" && (" << command << ")";

    int ret = bp::system(
                bp::search_path("ssh"),
                bp::args({
                   server(),
                   "bash -lc \""+escapeShellSymbols(cmd.str())+"\""
                })
          );

    cout<<"cmd string: >>>"<<cmd.str()<<"<<<"<<endl;

    if ( throwOnFail && (ret != 0) )
    {
        throw insight::Exception("Could not execute command on server "+server()+": \""+cmd.str()+"\"");
    }

    return ret;
}


void RemoteLocation::putFile
(
    const boost::filesystem::path& localFile,
    const boost::filesystem::path& remoteFileName,
    std::function<void(int,const std::string&)> pf
    )
{
  CurrentExceptionContext ex("put file "+localFile.string()+" to remote location");
  assertValid();

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
  CurrentExceptionContext ex("upload local directory "+localDir.string()+" to remote location");
  assertValid();

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
  CurrentExceptionContext ex("download remote files to local directory");
  assertValid();

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

void RemoteLocation::writeConfigFile(
    const filesystem::path &cfgf,
    const string &server,
    const filesystem::path &remoteDir,
    const filesystem::path &launchScript
    )
{
  using namespace rapidxml;

  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "remote");

  if (!launchScript.empty())
    rootnode->append_attribute(doc.allocate_attribute
                               ("launchScript",
                                 doc.allocate_string(launchScript.c_str())
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("server",
                                 doc.allocate_string(server.c_str())
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("directory",
                                 doc.allocate_string(remoteDir.c_str())
                                 )
                               );

  doc.append_node(rootnode);

  ofstream f(cfgf.c_str());
  f << doc;
}

bool RemoteLocation::isValid() const
{
  return isValid_;
}



bool RemoteLocation::remoteDirExists() const
{
  CurrentExceptionContext ex("check, if remote location exists");

  if (server_.empty() || remoteDir_.empty())
    return false;

  int ret = bp::system(
              bp::search_path("ssh"),
              bp::args({server(), "cd", remoteDir().string()})
        );

  if (ret==0)
    return true;
  else
    return false;
}




// ====================================================================================
// ======== RemoteExecutionConfig



boost::filesystem::path RemoteLocation::socket() const
{
  return remoteDir()/"tsp.socket";
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


RemoteExecutionConfig::RemoteExecutionConfig(const RemoteServerInfo &rsi,
                                             const filesystem::path &location,
                                             const filesystem::path &remoteRelPath,
                                             const filesystem::path &localREConfigFile)
  : RemoteLocation(rsi, remoteRelPath),
    localDir_(location)
{
  writeConfigFile(
        localREConfigFile.empty() ? defaultConfigFile(location) : localREConfigFile,
        server(),
        remoteDir(),
        rsi.hasLaunchScript_ ? rsi.server_ : ""
                               );
}

RemoteExecutionConfig::~RemoteExecutionConfig()
{}


const boost::filesystem::path& RemoteExecutionConfig::localDir() const
{
  return localDir_;
}

const boost::filesystem::path& RemoteExecutionConfig::metaFile() const
{
  return localREConfigFile_;
}

void RemoteExecutionConfig::cleanup()
{
  RemoteLocation::cleanup();
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





boost::filesystem::path RemoteExecutionConfig::defaultConfigFile(const boost::filesystem::path& location)
{
  return location/"meta.foam";
}

}
