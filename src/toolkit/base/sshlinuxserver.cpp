#include "sshlinuxserver.h"

#include <cstdlib>
#include <regex>

#include "base/exception.h"
#include "base/rapidxml.h"
#include "base/tools.h"
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
     boost::filesystem::path(e->first_attribute("baseDirectory")->value()),
     ( e->first_attribute("np")?
               insight::toNumber<int>(e->first_attribute("np")->value())
               : 1 )
     )
{
  hostName_=e->first_attribute("host")->value();

  if (auto ac = e->first_attribute("creationCommand"))
  {
      creationCommand_=ac->value();
  }
  if (auto dc = e->first_attribute("destructionCommand"))
  {
      destructionCommand_=dc->value();
  }
  if (creationCommand_.empty() != destructionCommand_.empty())
  {
      throw insight::Exception(
          "allocation and deallocation commands must be both specified!"
          );
  }
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
  appendAttribute(doc, *e, "label", *this );
  appendAttribute(doc, *e, "type", "SSHLinux" );
  appendAttribute(doc, *e, "host", hostName_ );
  appendAttribute(doc, *e, "baseDirectory", defaultDirectory_.string() );
  if (!creationCommand_.empty())
      appendAttribute(doc, *e, "creationCommand", creationCommand_ );
  if (!destructionCommand_.empty())
      appendAttribute(doc, *e, "destructionCommand", destructionCommand_ );
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

  RSyncOutputAnalyzer rpa(pf);
  auto job = std::make_shared<Job>("rsync", args);
  job->ios_run_with_interruption(&rpa);
  job->wait();

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






SSHLinuxServer::BackgroundJob::BackgroundJob(RemoteServer &server, int remotePid)
  : RemoteServer::BackgroundJob(server),
    remotePid_(remotePid)
{}

void SSHLinuxServer::BackgroundJob::kill()
{
  server_.executeCommand(
        boost::str(boost::format
         ( "if ps -q %d >/dev/null; then kill %d; fi" )
          % remotePid_ % remotePid_
        ),
        true
        );
}


RemoteServer::BackgroundJobPtr SSHLinuxServer::launchBackgroundProcess(
        const std::string &cmd,
        const std::vector<ExpectedOutput>& eobd )
{
  boost::process::ipstream is;

  auto process = launchCommand(
        cmd+" & echo PID===$!===PID",
//#ifdef WIN32
        boost::process::std_out > is
//#else
//        boost::process::std_err > is
//#endif
        , boost::process::std_in < boost::process::null
        );

  insight::assertion(
              process->running(),
              "could not start background process");

  std::vector<std::string> pidMatch;

  std::vector<ExpectedOutput> pats(eobd.begin(), eobd.end());
  pats.push_back( { boost::regex("PID===([0-9]+)===PID"), &pidMatch } );
  lookForPattern(is, pats);

  std::cout<<pidMatch[1]<<std::endl;
  int remotePid=boost::lexical_cast<int>(pidMatch[1]);

  insight::dbg()<<"remote process PID = "<<remotePid<<std::endl;

  process->detach();

  return std::make_shared<BackgroundJob>(*this, remotePid );
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

//  args.push_back( server() );

  SSHCommand sc(cfg.hostName_, args);
  tunnelProcess_=
      boost::process::child
      (
       sc.command(), boost::process::args( sc.arguments() )
      )
   ;
}




SSHLinuxServer::SSHTunnelPortMapping::~SSHTunnelPortMapping()
{}




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
