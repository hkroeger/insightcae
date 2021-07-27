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
    autoCreateRemoteDir_(orec.autoCreateRemoteDir_),
    isTemporaryStorage_(orec.isTemporaryStorage_),
    isValidated_(orec.isValidated_)
{}




RemoteLocation::RemoteLocation(const boost::filesystem::path& mf)
  : autoCreateRemoteDir_(false),
    isTemporaryStorage_(false),
    isValidated_(false)
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

    if (auto s = rootnode->first_attribute("temporary"))
    {
      std::string v(s->value());
      boost::to_lower(v);
      isTemporaryStorage_ = (v=="1")||(v=="yes")||(v=="on");
    }
    else
      insight::Warning("no temporary storage information configured for remote location. Assuming non-temporary storage.");

    if (auto s = rootnode->first_attribute("autocreate"))
    {
      std::string v(s->value());
      boost::to_lower(v);
      autoCreateRemoteDir_ = (v=="1")||(v=="yes")||(v=="on");
    }
    else
      insight::Warning("no auto-create storage flag configured for remote location. Assuming no auto creation.");

    if (auto rd = rootnode->first_attribute("directory"))
      remoteDir_ = rd->value();

    serverInstance_ = serverConfig_->getInstanceIfRunning();

    validate();

    if (!isValidated_)
    {
      throw insight::Exception("Remote execution configuration is invalid!");
    }
  }
}




RemoteLocation::RemoteLocation(
    RemoteServer::ConfigPtr rsc,
    const boost::filesystem::path& remotePath,
    bool autoCreateRemoteDir,
    bool isTemporaryStorage
    )
  : serverConfig_( rsc ),
    remoteDir_( remotePath ),
    autoCreateRemoteDir_( autoCreateRemoteDir ),
    isTemporaryStorage_( isTemporaryStorage ),
    isValidated_(false)
{
  if (remoteDir_.empty())
  {
    autoCreateRemoteDir_=true;
    isTemporaryStorage_=true;

    serverInstance_ = serverConfig_->getInstanceIfRunning();

    if (serverInstance_)
    {
      remoteDir_ = serverInstance_->getTemporaryDirectoryName(
            serverConfig()->defaultDirectory_/"irXXXXXX" );
    }
    else
    {
      throw insight::Exception("remote instance is not running: cannot determine suitable temporary storage");
    }
  }
}




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
  assertValid();

  execRemoteCmd("tsp -K");

  if (isTemporaryStorage())
    removeRemoteDir();
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
  if (!isValidated_)
  {
    CurrentExceptionContext ex("initializing remote location");

    if (!serverInstance_)
      serverInstance_ = serverConfig_->instance();

    if (autoCreateRemoteDir_)
    {

      if (!remoteDirExists())
      {
        serverInstance_->createDirectory(remoteDir_);
      }

    }
  }

  validate();
}




void RemoteLocation::validate()
{
  isValidated_=false;
  if (serverInstance_ && !remoteDir_.empty())
  {
    if (server()->hostIsAvailable())
    {
      if (remoteDirExists())
        isValidated_=true;
    }
  }
}




void RemoteLocation::removeRemoteDir()
{
  if (remoteDirExists())
  {
    serverInstance_->removeDirectory( remoteDir_ );
    isValidated_=false;
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




void RemoteLocation::assertValid() const
{
  if (!isValidated_)
    throw insight::Exception("Error: attempted communication with invalid remote location!");
}





std::vector<bfs_path> RemoteLocation::remoteLS() const
{
  CurrentExceptionContext ex("list remote directory contents");
  assertValid();

  return serverInstance_->listRemoteDirectory(remoteDir_);
}




std::vector<bfs_path> RemoteLocation::remoteSubdirs() const
{
  CurrentExceptionContext ex("list remote subdirectories");
  assertValid();

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
  assertValid();

  boost::filesystem::path rf=remoteFileName;
  if (rf.is_relative()) rf=remoteDir_/rf;

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
  assertValid();

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
  assertValid();

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

  server()->syncToLocal(localDir, remoteDir_, excl, pf);
}




void RemoteLocation::writeConfigFile(
    const filesystem::path &cfgf
    ) const
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
                                 doc.allocate_string(serverConfig()->c_str())
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("temporary",
                                 doc.allocate_string(
                                   isTemporaryStorage() ?
                                    "yes" : "no"
                                  )
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("autocreate",
                                 doc.allocate_string(
                                   autoCreateRemoteDir_ ?
                                    "yes" : "no"
                                  )
                                 )
                               );
  rootnode->append_attribute(doc.allocate_attribute
                               ("directory",
                                 doc.allocate_string(remoteDir().string().c_str())
                                 )
                               );

  doc.append_node(rootnode);

  ofstream f(cfgf.string());
  f << doc;
}




bool RemoteLocation::isActive() const
{
  return isValidated_;
}

bool RemoteLocation::isTemporaryStorage() const
{
  return isTemporaryStorage_;
}




bool RemoteLocation::remoteDirExists() const
{
  CurrentExceptionContext ex("check, if remote location exists");

  if (!serverInstance_ || remoteDir_.empty())
    return false;

  return serverInstance_->checkIfDirectoryExists(remoteDir_);
}




} // namespace insight
