#include "remotelocation.h"


#include <cstdlib>

#include "base/exception.h"
#include "base/tools.h"
#include "base/remoteserverlist.h"

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










RemoteLocation::RemoteLocation(const RemoteLocation& orec)
  : serverConfig_(orec.serverConfig_),
    serverInstance_(orec.serverInstance_),
    remoteDir_(orec.remoteDir_),
    emptyRemotePathSupplied_(orec.emptyRemotePathSupplied_),
    autoCreateRemoteDir_(orec.autoCreateRemoteDir_),
    isActive_(orec.isActive_)
{}




RemoteLocation::RemoteLocation(const boost::filesystem::path& mf)
  : emptyRemotePathSupplied_(false),
    autoCreateRemoteDir_(false)
{
  CurrentExceptionContext ce("reading configuration for remote execution in  directory "+mf.parent_path().string()+" from file "+mf.string());

  if (!boost::filesystem::exists(mf))
  {
    throw insight::Exception("There is no remote execution configuration file present!");
  }
  else
  {
    // read xml
    string content;
    readFileIntoString(mf, content);

    using namespace rapidxml;
    xml_document<> doc;
    doc.parse<0>(&content[0]);

    auto *rootnode = doc.first_node();
    if (!rootnode || rootnode->name()!=string("remote") )
      throw insight::Exception("No valid \"remote\" node found in XML!");

    if (auto s = rootnode->first_attribute("server"))
    {
      serverConfig_=remoteServers.findServer(std::string(s->value()));

    }
    else
      throw insight::Exception("no server name was configured for remote location");

    if (auto rd = rootnode->first_attribute("directory"))
      remoteDir_ = rd->value();

    serverInstance_ = serverConfig_->getInstanceIfRunning();

    validate();

    if (!isActive_)
    {
      throw insight::Exception("Remote execution configuration is invalid!");
    }
  }
}




RemoteLocation::RemoteLocation(RemoteServer::ConfigPtr rsc, const boost::filesystem::path& remotePath, bool autoCreateRemoteDir)
  : serverConfig_( rsc ),
    emptyRemotePathSupplied_(remotePath.empty()),
    autoCreateRemoteDir_( /*remotePath.empty()*/autoCreateRemoteDir )
{
  serverInstance_ = serverConfig_->getInstanceIfRunning();

  if (!remotePath.empty())
  {
    remoteDir_ = remotePath;
  }
  else
  {
    remoteDir_ = rsc->defaultDirectory_;
  }

  initialize();
}




RemoteLocation::~RemoteLocation()
{
//  stopTunnels();
}




//void RemoteLocation::createTunnels(
//    std::vector<boost::tuple<int, string, int> > remoteListenPorts,
//    std::vector<boost::tuple<int, string, int> > localListenPorts
//    )
//{
//  std::vector<std::string> args = { "-N" };

//  for (const auto& rlp: remoteListenPorts)
//  {
//    args.insert(
//          std::end(args),
//          {"-R", str(format("%d:%s:%d") % get<0>(rlp) % get<1>(rlp) % get<2>(rlp)) }
//    );
//  }

//  for (const auto& llp: localListenPorts)
//  {
//    args.insert(
//          std::end(args),
//          {"-L", str(format("%d:%s:%d") % get<0>(llp) % get<1>(llp) % get<2>(llp)) }
//    );
//  }

////  args.push_back( server() );

//  SSHCommand sc(server(), args);
//  tunnelProcesses_.push_back(
//      bp::child
//      (
//       sc.command(), bp::args( sc.arguments() )
//      )
//   );
//}




//void RemoteLocation::stopTunnels()
//{
//  size_t i=0;
//  for (auto& t: tunnelProcesses_)
//  {
//    i++;
//    cout << "Stopping tunnel instance "<<i<<"/"<<tunnelProcesses_.size()<<"..." << endl;
//    //try graceful exit
//#ifndef WIN32
//    kill(t.id(), SIGTERM);
//#else
//#warning implement WIN32 kill!
//#endif
//    if (!t.wait_for( std::chrono::seconds(10) ))
//    {
//      t.terminate();
//    }

//  }
//  tunnelProcesses_.clear();
//}




RemoteServerPtr RemoteLocation::server() const
{
  return serverInstance_;
}

RemoteServer::ConfigPtr RemoteLocation::serverConfig() const
{
  return serverConfig_;
}




const boost::filesystem::path& RemoteLocation::remoteDir() const
{
  return remoteDir_;
}




void RemoteLocation::cleanup()
{
  CurrentExceptionContext ex("clean remote server");
  assertActive();

  execRemoteCmd("tsp -K");

  removeRemoteDir();
//  disposeHost();
}




//void RemoteLocation::launchHost()
//{
//  if (!launchScript_.empty())
//  {
//    CurrentExceptionContext ce("executing launch script");

//    bp::ipstream out;

//    int ret = bp::system(
//          bp::search_path("sh"),
//          bp::args({ "-c",
//                     collectIntoSingleCommand(launchScript_.string(), {"launch"})
//                   }),
//          bp::std_out > out
//          );

//    if (ret!=0)
//    {
//      throw insight::Exception(
//            str( format("Failed to execute launch script %s for remote host!") % launchScript_)
//            );
//    }
//    else
//    {
//      getline(out, server_);
//    }
//  }

//  if (!serverIsUp())
//    throw insight::Exception("Could not establish connection to server "+server_+"!");
//}




void RemoteLocation::initialize()
{
  if (!serverInstance_)
    serverInstance_ = serverConfig_->instance();

  if (autoCreateRemoteDir_)
  {
    if (emptyRemotePathSupplied_)
    {
      remoteDir_ = serverInstance_->createTemporaryDirectory(remoteDir_/"irXXXXXX");
    }
    else if (!remoteDirExists())
    {
      serverInstance_->createDirectory(remoteDir_);
    }
  }
  validate();
}




void RemoteLocation::validate()
{
  isActive_=false;
  if (serverInstance_ && !remoteDir_.empty())
  {
    if (server()->hostIsAvailable())
    {
      if (remoteDirExists())
        isActive_=true;
    }
  }
}




void RemoteLocation::removeRemoteDir()
{
  if (remoteDirExists())
  {
    serverInstance_->removeDirectory( remoteDir_ );
    isActive_=false;
  }
}




//void RemoteLocation::disposeHost()
//{
//  if (!launchScript_.empty())
//  {
//    CurrentExceptionContext ce("executing dispose script");

//    int ret = bp::system(
//          bp::search_path("sh"),
//          bp::args({ "-c",
//                     collectIntoSingleCommand(launchScript_.string(), {"dispose", server()})
//                   })
//          );

//    if (ret!=0)
//    {
//      throw insight::Exception(
//            str( format("Failed to remote remote directory!") % launchScript_ )
//            );
//    }

//    isValid_=false;
//  }
//}




void RemoteLocation::assertActive() const
{
  if (!isActive_)
    throw insight::Exception("Error: attempted communication with invalid remote location!");
}





std::vector<bfs_path> RemoteLocation::remoteLS() const
{
  CurrentExceptionContext ex("list remote directory contents");
  assertActive();

  return serverInstance_->listRemoteDirectory(remoteDir_);
}




std::vector<bfs_path> RemoteLocation::remoteSubdirs() const
{
  CurrentExceptionContext ex("list remote subdirectories");
  assertActive();

  return serverInstance_->listRemoteSubdirectories(remoteDir_);
}




std::string RemoteLocation::remoteSourceOFEnvStatement() const
{
  try
  {
     const OFEnvironment& cofe = OFEs::getCurrent();
     return "source " + cofe.bashrc().filename().string() + ";";
  }
  catch (const std::exception& e)
  {
    try
    {
      const OFEnvironment& cofe = OFEs::getCurrentOrPreferred();

      WarningDispatcher::getCurrent().issue(
            "Could not detect currently loaded OpenFOAM environment. "
            "Running remote command with default OpenFOAM environment loaded.");

      return "source " + cofe.bashrc().filename().string() + ";";
    }
    catch (...)
    {
      WarningDispatcher::getCurrent().issue(
            "Could not detect any usable OpenFOAM environment. "
            "Running remote command with no OpenFOAM environment loaded.");
    }
  }

  return "";
}




void RemoteLocation::putFile
(
    const boost::filesystem::path& localFile,
    const boost::filesystem::path& remoteFileName,
    std::function<void(int,const std::string&)> pf
    )
{
  CurrentExceptionContext ex("put file "+localFile.string()+" to remote location");
  assertActive();

  boost::filesystem::path rf=remoteFileName;
  if (rf.is_relative()) rf=remoteDir_/rf;
//  std::vector<std::string> args=
//      {
//       lf.string(),
//       server_+":"+rf.string()
//      };

//  runRsync(args, pf);
  server()->putFile(localFile, rf, pf);
}




void RemoteLocation::syncToRemote
(
    const boost::filesystem::path& localDir,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
  CurrentExceptionContext ex("upload local directory "+localDir.string()+" to remote location");
  assertActive();

//    std::vector<std::string> args=
//        {
//         "-az",
//         "--delete",
//         "--info=progress",

//         "--exclude", "processor*",
//         "--exclude", "*.foam",
//         "--exclude", "postProcessing",
//         "--exclude", "*.socket",
//         "--exclude", "backup",
//         "--exclude", "archive",
//         "--exclude", "mnt_remote"
//        };

//    for (const auto& ex: exclude_pattern)
//    {
//      args.push_back("--exclude");
//      args.push_back(ex);
//    }

//    args.push_back(localDir.string()+"/");
//    args.push_back(server_+":"+remoteDir_.string());

//    runRsync(args, pf);

    server()->syncToRemote(localDir, remoteDir_, exclude_pattern, pf);
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
  assertActive();

//  std::vector<std::string> args;

//    args =
//    {
//      "-az",
//      "--info=progress",
//      "--exclude", "processor*",
//      "--exclude", "*.foam",
//      "--exclude", "*.socket",
//      "--exclude", "backup",
//      "--exclude", "archive",
//      "--exclude", "mnt_remote"
//    };

    std::vector<std::string> excl = exclude_pattern;
    if (skipTimeSteps)
      {
        auto files = remoteLS();

        // remove non-numbers
        files.erase(remove_if(files.begin(), files.end(),
                [&](const bfs_path& f)
                {
                  try { to_number<double>(f.string()); return false; }
                  catch (...) { return true; }
                }), files.end());

        for (const auto& f: files)
          {
            excl.push_back(f.string());
          }
      }

//    for (const auto& ex: exclude_pattern)
//    {
//      args.push_back("--exclude");
//      args.push_back(ex);
//    }

//    args.push_back(server_+":"+remoteDir_.string()+"/");
//    args.push_back(localDir.string());

//    runRsync(args, pf);

    server()->syncToLocal(localDir, remoteDir_, excl, pf);
}




void RemoteLocation::writeConfigFile(
    const filesystem::path &cfgf,
    const string &server,
    const filesystem::path &remoteDir
    )
{
  using namespace rapidxml;

  xml_document<> doc;
  xml_node<>* decl = doc.allocate_node(node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  decl->append_attribute(doc.allocate_attribute("encoding", "utf-8"));
  doc.append_node(decl);
  xml_node<> *rootnode = doc.allocate_node(node_element, "remote");


  rootnode->append_attribute(doc.allocate_attribute
                               ("server",
                                 doc.allocate_string(server.c_str())
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("directory",
                                 doc.allocate_string(remoteDir.string().c_str())
                                 )
                               );

  doc.append_node(rootnode);

  ofstream f(cfgf.c_str());
  f << doc;
}




bool RemoteLocation::isActive() const
{
  return isActive_;
}




bool RemoteLocation::remoteDirExists() const
{
  CurrentExceptionContext ex("check, if remote location exists");

  if (!serverInstance_ || remoteDir_.empty())
    return false;

  return serverInstance_->checkIfDirectoryExists(remoteDir_);
}




} // namespace insight
