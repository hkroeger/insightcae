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
    const std::string& launchCommand,
    const std::string& shutdownCommand )
  : LinuxRemoteServer::Config(bp, np),
    hostName_(hostName),
    launchCommand_(launchCommand),
    shutdownCommand_(shutdownCommand)
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

  if (auto ac = e->first_attribute("launchCommand"))
  {
      launchCommand_=ac->value();
  }
  if (auto dc = e->first_attribute("shutdownCommand"))
  {
      shutdownCommand_=dc->value();
  }
  if (auto a = e->first_attribute("creationCommand"))
      creationCommand_=a->value();
  if (auto a = e->first_attribute("destructionCommand"))
      destructionCommand_=a->value();
  if (auto a = e->first_attribute("existenceCheckCommand"))
      existenceCheckCommand_=a->value();
  launchTimeout_ = std::chrono::seconds(
      getOptionalAttributeOrDefault(
          *e, "launchTimeout", int(defaultLaunchTimeout().count()) ) );

  if (launchCommand_.empty() != shutdownCommand_.empty())
  {
      throw insight::Exception(
          "launch and shutdown commands must be both specified!"
          );
  }

  int nCreationCommands =
      int(!creationCommand_.empty())
      + int(!destructionCommand_.empty())
      + int(!existenceCheckCommand_.empty());
  if (nCreationCommands!=0 && nCreationCommands!=3)
  {
      throw insight::Exception(
          "creation, destruction and existence check commands must be all specified!"
          );
  }
}

std::chrono::seconds SSHLinuxServer::Config::defaultLaunchTimeout()
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

bool SSHLinuxServer::Config::isRunning() const
{
    CurrentExceptionContext ex("checking, if server %s is reachable", c_str());

    std::vector<std::string> args;
#ifndef WIN32
    args = { "-o", "ConnectTimeout=5", "-o", "BatchMode=yes" };
#endif
    args.push_back("exit");

    SSHCommand ssh(hostName_, args);
    boost::process::child c(
        ssh.command(), boost::process::args(ssh.arguments()),
        boost::process::std_in < boost::process::null );
    c.wait();
    return c.exit_code()==0;
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
    if (!launchCommand_.empty())
        appendAttribute(doc, *e, "launchCommand", launchCommand_ );
    if (!shutdownCommand_.empty())
        appendAttribute(doc, *e, "shutdownCommand", shutdownCommand_ );
    if (!creationCommand_.empty())
        appendAttribute(doc, *e, "creationCommand", creationCommand_ );
    if (!destructionCommand_.empty())
        appendAttribute(doc, *e, "destructionCommand", destructionCommand_ );
    if (!existenceCheckCommand_.empty())
        appendAttribute(doc, *e, "existenceCheckCommand", existenceCheckCommand_ );
    if (launchTimeout_!=defaultLaunchTimeout())
        appendAttribute(doc, *e, "launchTimeout", int(launchTimeout_.count()) );
}

RemoteServer::ConfigPtr SSHLinuxServer::Config::clone() const
{
    return std::make_shared<Config>(*this);
}



bool SSHLinuxServer::Config::isExpandable() const
{
    for (const auto* s: { &hostName_, &launchCommand_, &shutdownCommand_,
                          &creationCommand_, &destructionCommand_, &existenceCheckCommand_ })
    {
        if (s->find("%d")!=std::string::npos) return true;
    }
    return false;
}

RemoteServer::ConfigPtr SSHLinuxServer::Config::expanded(int id) const
{
    auto cp = std::make_shared<SSHLinuxServer::Config>(*this);
    cp->originatedFromExpansion_=true;

    // plain replacement: boost::format would not accept the %{...} placeholders
    auto sid = boost::lexical_cast<std::string>(id);
    for (auto* s: { &cp->hostName_, &cp->launchCommand_, &cp->shutdownCommand_,
                    &cp->creationCommand_, &cp->destructionCommand_, &cp->existenceCheckCommand_ })
    {
        boost::replace_all(*s, "%d", sid);
    }

    cp->std::string::operator=(
        *cp+"/"+cp->hostName_ );

    return cp;
}


std::string SSHLinuxServer::Config::expandPlaceholders(const std::string& command) const
{
    std::string r(command);
    boost::replace_all(r, "%{serverLabel}", static_cast<const std::string&>(*this));
    boost::replace_all(r, "%{host}", hostName_);
    return r;
}


bool SSHLinuxServer::Config::canBeLaunched() const
{
    return !launchCommand_.empty() || canBeCreated();
}

bool SSHLinuxServer::Config::canBeShutDown() const
{
    return !shutdownCommand_.empty();
}

bool SSHLinuxServer::Config::canBeCreated() const
{
    return !creationCommand_.empty() && !existenceCheckCommand_.empty();
}

bool SSHLinuxServer::Config::canBeDisposed() const
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




namespace {

void runShellCommand(
    const std::string& cmd,
    const RemoteServer::Config::OutputLineCallback& outputLine,
    int* exitCode )
{
    auto job= Job::forkExternalProcess("bash", {"-lc", cmd});

    if (outputLine)
    {
        job->ios_run_with_interruption(
            [&](const std::string& line) { outputLine(line); },
            [&](const std::string& line) { outputLine("[E] "+line); } );
        job->wait(); // exit code is not set correctly, if this is skipped
    }
    else
    {
        job->runAndTransferOutput();
    }

    *exitCode = job->process().exit_code();
}

}




int SSHLinuxServer::Config::runLocalCommand(
    const std::string& command, OutputLineCallback outputLine ) const
{
    auto cmd = expandPlaceholders(command);
    if (outputLine) outputLine("$ "+cmd); else std::cout<<"$ "<<cmd<<std::endl;
    int retcode=-1;
    runShellCommand(cmd, outputLine, &retcode);
    return retcode;
}




bool SSHLinuxServer::Config::waitUntilReachable(OutputLineCallback outputLine) const
{
    auto msg = [&](const std::string& line)
    {
        if (outputLine) outputLine(line); else std::cout<<line<<std::endl;
    };

    msg(str(format("Waiting for %s to become reachable (at most %d s)...")
            % hostName_ % int(launchTimeout_.count()) ));
    auto deadline = std::chrono::steady_clock::now() + launchTimeout_;
    while (!isRunning())
    {
        if (std::chrono::steady_clock::now() >= deadline)
            return false;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    msg(hostName_+" is reachable.");
    return true;
}




void SSHLinuxServer::Config::launch(OutputLineCallback outputLine) const
{
    if ( !canBeLaunched() || isRunning() )
        return;

    CurrentExceptionContext ex("launching server %s", c_str());

    auto msg = [&](const std::string& line)
    {
        if (outputLine) outputLine(line); else std::cout<<line<<std::endl;
    };

    // don't leave a (possibly costly) unusable instance behind
    bool created=false;
    auto cleanupAfterFailure = [&]()
    {
        if (created)
        {
            try
            {
                dispose(outputLine);
            }
            catch (std::exception& e)
            {
                insight::Warning("failed to dispose the execution server: %s", e.what());
            }
        }
        else
        {
            runShutdownCommand(outputLine);
        }
    };

    // create the instance, if it does not exist
    if (canBeCreated())
    {
        auto ex = exists();
        if (ex && !*ex)
        {
            msg("The instance does not exist, creating it...");
            int retcode = runLocalCommand(creationCommand_, outputLine);
            if (retcode!=0)
            {
                throw insight::Exception(
                    "failed to create the execution server (creation command exited with code %d)",
                    retcode );
            }
            created=true;

            if (waitUntilReachable(outputLine))
                return;

            if (launchCommand_.empty())
            {
                cleanupAfterFailure();
                throw insight::Exception(
                    "the execution server %s was created, but is not reachable after %d s",
                    hostName_.c_str(), int(launchTimeout_.count()) );
            }

            msg("The created instance is not reachable, executing the launch command...");
        }
    }

    if (launchCommand_.empty())
    {
        throw insight::Exception(
            "the execution server %s is not reachable and there is no launch command",
            hostName_.c_str() );
    }

    int retcode = runLocalCommand(launchCommand_, outputLine);
    if (retcode!=0)
    {
        cleanupAfterFailure();
        throw insight::Exception(
            "failed to launch execution server (launch command exited with code %d)",
            retcode );
    }

    if (!waitUntilReachable(outputLine))
    {
        cleanupAfterFailure();
        throw insight::Exception(
            "the execution server %s was launched, but is not reachable after %d s",
            hostName_.c_str(), int(launchTimeout_.count()) );
    }
}




void SSHLinuxServer::Config::shutdown(OutputLineCallback outputLine) const
{
    if (!canBeShutDown())
    {
        RemoteServer::Config::shutdown(outputLine); // throws
    }

    CurrentExceptionContext ex("shutting down server %s", c_str());

    int retcode = runLocalCommand(shutdownCommand_, outputLine);
    if (retcode!=0)
    {
        throw insight::Exception(
            "failed to shut down the execution server (shutdown command exited with code %d)",
            retcode );
    }

    std::string m = c_str()+std::string(" was shut down.");
    if (outputLine) outputLine(m); else std::cout<<m<<std::endl;
}




boost::optional<bool> SSHLinuxServer::Config::exists() const
{
    if (existenceCheckCommand_.empty())
        return boost::none;

    try
    {
        auto cmd = expandPlaceholders(existenceCheckCommand_);
        int retcode=-1;
        runShellCommand(
            cmd,
            [](const std::string& line) { insight::dbg()<<line<<std::endl; },
            &retcode );
        insight::dbg()<<"existence check of "<<c_str()<<": exit code "<<retcode<<std::endl;
        return retcode==0;
    }
    catch (std::exception& e)
    {
        insight::dbg()<<"existence check of "<<c_str()<<" failed: "<<e.what()<<std::endl;
    }
    return boost::none;
}




void SSHLinuxServer::Config::dispose(OutputLineCallback outputLine) const
{
    if (!canBeDisposed())
    {
        RemoteServer::Config::dispose(outputLine); // throws
    }

    CurrentExceptionContext ex("disposing server %s", c_str());

    if (canBeShutDown() && isRunning())
    {
        shutdown(outputLine);
    }

    int retcode = runLocalCommand(destructionCommand_, outputLine);
    if (retcode!=0)
    {
        throw insight::Exception(
            "failed to dispose the execution server (destruction command exited with code %d)",
            retcode );
    }

    std::string m = c_str()+std::string(" was disposed.");
    if (outputLine) outputLine(m); else std::cout<<m<<std::endl;
}




void SSHLinuxServer::Config::runShutdownCommand(OutputLineCallback outputLine) const
{
    if (!canBeShutDown()) return;

    try
    {
        CurrentExceptionContext ex("stopping server %s", c_str());

        int retcode = runLocalCommand(shutdownCommand_, outputLine);
        if (retcode!=0)
        {
            insight::Warning("failed to deallocate execution server");
        }
    }
    catch (std::exception& e)
    {
        insight::Warning("failed to deallocate execution server: %s", e.what());
    }
}




SSHLinuxServer::SSHLinuxServer(const Config& serverConfig)
    : serverConfig_(serverConfig),
    bwlimit_(-1)
{
    serverConfig_.launch();
}

void SSHLinuxServer::destroyIfPossible()
{
    serverConfig_.runShutdownCommand();
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
    std::function<void(int,const std::string&)> pf,
    const std::vector<boost::filesystem::path>& selection
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

    // restrict to the selected items (if any)
    {
      std::vector<std::string> selBefore, selAfter;
      rsyncSelectionFilter(selection, selBefore, selAfter);
      args.insert(args.begin()+1, selBefore.begin(), selBefore.end()); // after "-az"
      args.insert(args.end(), selAfter.begin(), selAfter.end());
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
    std::function<void(int,const std::string&)> pf,
    const std::vector<boost::filesystem::path>& selection
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

    // restrict to the selected items (if any)
    {
      std::vector<std::string> selBefore, selAfter;
      rsyncSelectionFilter(selection, selBefore, selAfter);
      args.insert(args.begin()+1, selBefore.begin(), selBefore.end()); // after "-az"
      args.insert(args.end(), selAfter.begin(), selAfter.end());
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
