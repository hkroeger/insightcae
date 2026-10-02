#include "sshlinuxserver.h"

#include <cstdlib>
#include <boost/asio.hpp>
#include <thread>
#include <fstream>
#include <regex>

#include "base/exception.h"
#include "base/rsyncoutputanalyzer.h"
#include "base/warningdispatcher.h"
#include "base/rapidxml.h"
#include "base/tools.h"
#include "base/remotecommand.h"
#include "boost/format/format_fwd.hpp"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"

using namespace std;
using namespace boost;

namespace insight
{


SSHLinuxServer::Config::Config(
    const boost::filesystem::path& bp,
    int np,
    const std::string hostName,
    const std::string& creationCommand,
    const std::string& destructionCommand )
  : LinuxRemoteServer::Config(bp, np),
    hostName_(hostName),
    creationCommand_(creationCommand),
    destructionCommand_(destructionCommand)
{}

SSHLinuxServer::Config::Config(
    rapidxml::xml_node<> *e
    )
  :LinuxRemoteServer::Config(
     getMandatoryAttribute<boost::filesystem::path>(*e, "baseDirectory"),
     getOptionalAttributeOrDefault(*e, "np", 1 )
    ),
   hostName_( getAttribute(*e, "host") )
{

  if (auto ac = e->first_attribute("creationCommand"))
  {
      creationCommand_=ac->value();
  }
  if (auto dc = e->first_attribute("destructionCommand"))
  {
      destructionCommand_=dc->value();
  }
  creationTimeout_ = std::chrono::seconds(
      getOptionalAttributeOrDefault(
          *e, "creationTimeout", int(defaultCreationTimeout().count()) ) );

  if (creationCommand_.empty() != destructionCommand_.empty())
  {
      throw insight::Exception(
          "allocation and deallocation commands must be both specified!"
          );
  }
}

std::chrono::seconds SSHLinuxServer::Config::defaultCreationTimeout()
{
    return std::chrono::seconds(300);
}

std::shared_ptr<RemoteServer> SSHLinuxServer::Config::instance() const
{
  return std::make_shared<SSHLinuxServer>( *this );
}


std::pair<boost::filesystem::path,std::vector<std::string> >
SSHLinuxServer::Config::commandAndArgs(const std::string& command) const
{
    std::string expr // = "bash -lc '"+command+"'";
        = command;

    SSHCommand ssh(hostName_, { expr });

    {
        auto& os = insight::dbg();
        os << ssh.command() << " ";
        for (const auto& a: ssh.arguments())
            os << " \"" + a + "\"";
        os << std::endl;
    }

    return { ssh.command(),
            ssh.arguments() };
}

bool SSHLinuxServer::Config::isDynamicallyAllocated() const
{
  return false;
}

void SSHLinuxServer::Config::save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const
{
    RemoteServer::Config::save(e, doc);
    appendAttribute(doc, *e, "type", "SSHLinux" );
    appendAttribute(doc, *e, "host", hostName_ );
    appendAttribute(doc, *e, "baseDirectory", defaultDirectory_.string() );
    if (!creationCommand_.empty())
        appendAttribute(doc, *e, "creationCommand", creationCommand_ );
    if (!destructionCommand_.empty())
        appendAttribute(doc, *e, "destructionCommand", destructionCommand_ );
    if (creationTimeout_!=defaultCreationTimeout())
        appendAttribute(doc, *e, "creationTimeout", int(creationTimeout_.count()) );
}

RemoteServer::ConfigPtr SSHLinuxServer::Config::clone() const
{
    return std::make_shared<Config>(*this);
}



bool SSHLinuxServer::Config::isExpandable() const
{
    return
        (hostName_.find("%d")!=std::string::npos)
        || (creationCommand_.find("%d")!=std::string::npos)
        || (destructionCommand_.find("%d")!=std::string::npos)
        ;
}

RemoteServer::ConfigPtr SSHLinuxServer::Config::expanded(int id) const
{
    auto cp = std::make_shared<SSHLinuxServer::Config>(*this);
    cp->originatedFromExpansion_=true;

    if (cp->hostName_.find("%d")
        != std::string::npos)
    {
        cp->hostName_=
            str(boost::format(cp->hostName_)%id);
    }

    cp->std::string::operator=(
        *cp+"/"+cp->hostName_ );

    if (cp->creationCommand_.find("%d")
        != std::string::npos)
    {
        cp->creationCommand_=
            str(boost::format(cp->creationCommand_)%id);
    }

    if (cp->destructionCommand_.find("%d")
        != std::string::npos)
    {
        cp->destructionCommand_=
            str(boost::format(cp->destructionCommand_)%id);
    }

    return cp;
}


bool SSHLinuxServer::Config::isDynamicallyCreatable() const
{
    return !creationCommand_.empty();
}

bool SSHLinuxServer::Config::isDynamicallyDestructable() const
{
    return !destructionCommand_.empty();
}


void SSHLinuxServer::runRsync
(
    const std::vector<std::string>& uargs,
    std::function<void(int,const std::string&)> pf
)
{
  assertRunning();

  std::vector<std::string> args(uargs);
  args.insert(args.begin(), "--info=progress2");

  auto job = std::make_shared<Job>("rsync", args);
  runRSync(*job, pf);
}




SSHLinuxServer::SSHLinuxServer(const Config& serverConfig)
    : serverConfig_(serverConfig),
    bwlimit_(-1)
{
    if (
        !serverConfig_.creationCommand_.empty()
         &&
        !config().isRunning() )
    {
        CurrentExceptionContext ex("launching server %s", serverLabel().c_str());

        auto job= Job::forkExternalProcess("bash", {"-lc", serverConfig_.creationCommand_});

        job->runAndTransferOutput();

        auto retcode = job->process().exit_code();
        if (retcode!=0)
        {
            throw insight::Exception(
                "failed to launch execution server"
                );
        }

        // wait, until the new server is reachable
        auto deadline = std::chrono::steady_clock::now() + serverConfig_.creationTimeout_;
        while (!config().isRunning())
        {
            if (std::chrono::steady_clock::now() >= deadline)
            {
                // don't leave a (possibly costly) unusable instance behind
                try
                {
                    SSHLinuxServer::destroyIfPossible();
                }
                catch (std::exception& e)
                {
                    insight::Warning("failed to deallocate execution server: %s", e.what());
                }

                throw insight::Exception(
                    "the execution server %s was created, but is not reachable after %d s",
                    hostName().c_str(), int(serverConfig_.creationTimeout_.count()) );
            }
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
}

void SSHLinuxServer::destroyIfPossible()
{
    if (serverConfig_.isDynamicallyDestructable())
    {
        CurrentExceptionContext ex("stopping server %s", serverLabel().c_str());

        auto job= Job::forkExternalProcess("bash", {"-lc", serverConfig_.destructionCommand_});

        job->runAndTransferOutput();

        auto retcode = job->process().exit_code();
        if (retcode!=0)
        {
            insight::Warning(
                "failed to deallocate execution server"
                );
        }
    }
}




const SSHLinuxServer::Config& SSHLinuxServer::SSHServerConfig() const
{
  return serverConfig_;
}


const RemoteServer::Config &SSHLinuxServer::config() const
{
    return SSHServerConfig();
}



string SSHLinuxServer::hostName() const
{
  return SSHServerConfig().hostName_;
}






void SSHLinuxServer::putFile
(
    const boost::filesystem::path& localFilePath,
    const boost::filesystem::path& remoteFilePath,
    std::function<void(int,const std::string&)> pf
    )
{
  CurrentExceptionContext ex("put file "+localFilePath.string()+" to remote location");
  assertRunning();

  std::vector<std::string> args=
      {
       localFilePath.string(),
       hostName()+":"+toUnixPath(remoteFilePath)
      };

  runRsync(args, pf);
}


void SSHLinuxServer::setTransferBandWidthLimit(int kBPerSecond)
{
    bwlimit_=kBPerSecond;
}

int SSHLinuxServer::transferBandWidthLimit() const
{
    return bwlimit_;
}

void SSHLinuxServer::syncToRemote
(
    const boost::filesystem::path& localDir,
    const boost::filesystem::path& remoteDir,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
  CurrentExceptionContext ex("upload local directory "+localDir.string()+" to remote location");
  assertRunning();

    std::vector<std::string> args=
        {
         "-az",
         "--delete",

         "--exclude", "*.foam",
         "--exclude", "postProcessing",
         "--exclude", "*.socket",
         "--exclude", "backup",
         "--exclude", "archive",
         "--exclude", "mnt_remote"
        };

    if (!includeProcessorDirectories)
    {
        args.push_back("--exclude");
        args.push_back("processor*");
    }

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    if (bwlimit_>0)
    {
        args.push_back(
                    str(format("--bwlimit=%d")
                        % bwlimit_ ));
    }

    args.push_back(localDir.string()+"/");
    args.push_back(hostName()+":"+toUnixPath(remoteDir));

    runRsync(args, pf);
}




void SSHLinuxServer::syncToLocal
(
    const boost::filesystem::path& localDir,
    const boost::filesystem::path& remoteDir,
    bool includeProcessorDirectories,
    const std::vector<std::string>& exclude_pattern,
    std::function<void(int,const std::string&)> pf
)
{
  CurrentExceptionContext ex("download remote files to local directory");
  assertRunning();

  std::vector<std::string> args;

    args =
    {
      "-az",
      //"--exclude", "processor*",
      "--exclude", "*.foam",
      "--exclude", "*.socket",
      "--exclude", "backup",
      "--exclude", "archive",
      "--exclude", "mnt_remote"
    };

    if (!includeProcessorDirectories)
    {
        args.push_back("--exclude");
        args.push_back("processor*");
    }

    for (const auto& ex: exclude_pattern)
    {
      args.push_back("--exclude");
      args.push_back(ex);
    }

    if (bwlimit_>0)
    {
        args.push_back(
                    str(format("--bwlimit=%d")
                        % bwlimit_ ));
    }

    args.push_back(hostName()+":"+toUnixPath(remoteDir)+"/");
    args.push_back(localDir.string());

    runRsync(args, pf);
}





RemoteServer::PortMappingPtr SSHLinuxServer::makePortsAccessible(
    const std::set<int> &remoteListenerPorts,
    const std::set<int> &localListenerPorts)
{
  insight::CurrentExceptionContext ex("create port tunnels via SSH");

  return std::make_shared<SSHTunnelPortMapping>(
        SSHServerConfig(),
        remoteListenerPorts,
        localListenerPorts
        );
}




SSHLinuxServer::SSHTunnelPortMapping::SSHTunnelPortMapping(
    const Config& cfg,
    const std::set<int>& remoteListenerPorts,
    const std::set<int>& localListenerPorts
    )
{
  std::vector<std::string> args = { "-N" };

  for (const auto& rlp: remoteListenerPorts)
  {
    int localPort = findFreePort();
    insight::dbg()<<"remoteListenerPort: "<<rlp<<" / "<<localPort<<std::endl;
    remoteToLocal_.insert( std::pair<int,int>(rlp, localPort) );
    args.insert(
          std::end(args),
          {"-L", str(format("%d:%s:%d") % localPort % "127.0.0.1" % rlp) }
    );
  }

  for (const auto& llp: localListenerPorts)
  {
    int remotePort = findRemoteFreePort(cfg.hostName_);
    insight::dbg()<<"localListenerPorts: "<<llp<<" / "<<remotePort<<std::endl;
    localToRemote_.insert( std::pair<int,int>(llp, remotePort) );
    args.insert(
          std::end(args),
          {"-R", str(format("%d:%s:%d") % remotePort % "127.0.0.1" % llp ) }
    );
  }

#ifndef WIN32
  // fail instead of hanging, if the forwarding cannot be established
  // or the connection breaks down
  args.insert(
        std::end(args),
        { "-o", "ExitOnForwardFailure=yes",
          "-o", "BatchMode=yes",
          "-o", "ServerAliveInterval=15",
          "-o", "ServerAliveCountMax=3" } );
#endif

  errorLog_ = boost::filesystem::temp_directory_path()
              / boost::filesystem::unique_path("insight-sshtunnel-%%%%%%%%.log");

  SSHCommand sc(cfg.hostName_, args);
  tunnelProcess_=
      boost::process::child
      (
       sc.command(), boost::process::args( sc.arguments() ),
       boost::process::std_in < boost::process::null,
       boost::process::std_out > boost::process::null,
       boost::process::std_err > errorLog_
      )
   ;

  try
  {
    waitUntilReady(std::chrono::seconds(20));
  }
  catch (...)
  {
    // the destructor is not called: clean up here
    std::error_code ec;
    if (tunnelProcess_.running(ec)) tunnelProcess_.terminate(ec);
    boost::system::error_code bec;
    boost::filesystem::remove(errorLog_, bec);
    throw;
  }
}




void SSHLinuxServer::SSHTunnelPortMapping::waitUntilReady(std::chrono::milliseconds timeout)
{
  auto errorOutput = [this]() -> std::string
  {
    std::string err;
    boost::system::error_code ec;
    if (boost::filesystem::exists(errorLog_, ec))
    {
      std::ifstream f(errorLog_.string());
      err.assign( std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>() );
      boost::trim(err);
    }
    return err;
  };

  auto deadline = std::chrono::steady_clock::now() + timeout;

  std::set<int> pendingPorts;
  for (const auto& rl: remoteToLocal_)
    pendingPorts.insert(rl.second);

  while (true)
  {
    std::error_code ec;
    if (!tunnelProcess_.running(ec))
    {
      throw insight::Exception(
            "the SSH tunnel could not be established: %s",
            errorOutput().c_str() );
    }

    // probe the local ends of the tunnels
    for (auto i=pendingPorts.begin(); i!=pendingPorts.end(); )
    {
      boost::asio::io_service ios;
      boost::asio::ip::tcp::socket sock(ios);
      boost::system::error_code cec;
      sock.connect(
            boost::asio::ip::tcp::endpoint(
                boost::asio::ip::address::from_string("127.0.0.1"), *i ),
            cec );
      if (!cec)
        i=pendingPorts.erase(i);
      else
        ++i;
    }

    if (pendingPorts.empty())
    {
      if (remoteToLocal_.empty())
      {
        // only remote forwards: they cannot be probed from here.
        // Give ssh some time to fail (ExitOnForwardFailure).
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!tunnelProcess_.running(ec))
          continue; // report the failure
      }
      return;
    }

    if (std::chrono::steady_clock::now() > deadline)
    {
      throw insight::Exception(
            "the SSH tunnel was not ready within %d s: %s",
            int(std::chrono::duration_cast<std::chrono::seconds>(timeout).count()),
            errorOutput().c_str() );
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}




SSHLinuxServer::SSHTunnelPortMapping::~SSHTunnelPortMapping()
{
  std::error_code ec;
  if (tunnelProcess_.running(ec))
  {
    tunnelProcess_.terminate(ec);
  }
  if (tunnelProcess_.valid())
  {
    tunnelProcess_.wait(ec);
  }
  boost::system::error_code bec;
  boost::filesystem::remove(errorLog_, bec);
}




int SSHLinuxServer::SSHTunnelPortMapping::localListenerPort(int remoteListenerPort) const
{
  insight::CurrentExceptionContext ex(
        boost::str(boost::format("returning local port to remote listener port %d")%remoteListenerPort));
  return remoteToLocal_.at(remoteListenerPort);
}




int SSHLinuxServer::SSHTunnelPortMapping::remoteListenerPort(int localListenerPort) const
{
  insight::CurrentExceptionContext ex(
        boost::str(boost::format("returning remote port to local listener port %d")%localListenerPort));
  return localToRemote_.at(localListenerPort);
}



}
