#include "wsllinuxserver.h"

#include <cstdlib>
#include <regex>
#include <codecvt>

#include "base/exception.h"
#include "base/tools.h"
#include "base/shelltools.h"
#include "openfoam/openfoamcase.h"

#include "rapidxml/rapidxml_print.hpp"


using namespace std;
using namespace boost;

namespace insight {




WSLLinuxServer::Config::Config(
        const boost::filesystem::path& bp,
        int np,
        const std::string& distributionLabel )
  : LinuxRemoteServer::Config(bp, np),
    distributionLabel_(distributionLabel)
{}




WSLLinuxServer::Config::Config(rapidxml::xml_node<> *e)
  :LinuxRemoteServer::Config(
     boost::filesystem::path(e->first_attribute("baseDirectory")->value()),
      ( e->first_attribute("np")?
           insight::toNumber<int>(e->first_attribute("np")->value())
                                : 1 )
     )
{
  auto* ha = e->first_attribute("distributionLabel");

  distributionLabel_=ha->value();
}






std::shared_ptr<RemoteServer> WSLLinuxServer::Config::instance() const
{
  return std::make_shared<WSLLinuxServer>( *this );
}


std::pair<boost::filesystem::path,std::vector<std::string> >
WSLLinuxServer::Config::commandAndArgs(const std::string& command) const
{
    return {
        WSLcommand(),
        { "-d", distributionLabel_,
         "--", "bash", "-lc", command }
    };
}


bool WSLLinuxServer::Config::isDynamicallyAllocated() const
{
  return false;
}




void WSLLinuxServer::Config::save(rapidxml::xml_node<> *e, rapidxml::xml_document<>& doc) const
{
  e->append_attribute( doc.allocate_attribute( "label", this->c_str() ) );
  e->append_attribute( doc.allocate_attribute( "type", "WSLLinux" ) );
  e->append_attribute( doc.allocate_attribute( "distributionLabel",doc.allocate_string(
                                               distributionLabel_.c_str() ) ) );
  e->append_attribute( doc.allocate_attribute( "baseDirectory", doc.allocate_string(
                                               defaultDirectory_.string().c_str() ) ) );
}

RemoteServer::ConfigPtr WSLLinuxServer::Config::clone() const
{
    return std::make_shared<WSLLinuxServer::Config>(*this);
}


bool WSLLinuxServer::Config::isDynamicallyCreatable() const
{
    return false;
}


bool WSLLinuxServer::Config::isDynamicallyDestructable() const
{
    return false;
}



WSLLinuxServer::WSLLinuxServer(const Config& serverConfig)
  : serverConfig_(serverConfig)
{
}


const RemoteServer::Config& WSLLinuxServer::config() const
{
    return serverConfig_;
}

const WSLLinuxServer::Config& WSLLinuxServer::WSLServerConfig() const
{
  return dynamic_cast<const Config&>(config());
}


boost::filesystem::path WSLLinuxServer::WSLcommand()
{
  std::string windir("C:\\Windows");
  if (auto *wdv=getenv("WINDIR"))
  {
    windir=wdv;
  }
  auto wsl =boost::filesystem::path(windir+"\\Sysnative\\wsl.exe"); // weird: system32 does not work...

  insight::dbg()<<"WSL executable: "<<wsl<<std::endl;
  return wsl;
}





WSLLinuxServer::BackgroundJob::BackgroundJob(
    RemoteServer &server,
    std::unique_ptr<boost::process::child> process )
  : RemoteServer::BackgroundJob(server),
    process_(std::move(process))
{}




void WSLLinuxServer::BackgroundJob::kill()
{
  if (process_)
  {
    process_->terminate();
    process_->wait();
    process_.reset();
  }
}




RemoteServer::BackgroundJobPtr WSLLinuxServer::launchBackgroundProcess(
        const std::string &cmd,
        const std::vector<ExpectedOutput>& pattern )
{
    insight::assertion(
        pattern.size()==0,
        "Not implemented: cannot look for pattern in WSL process");

    //  lookForPattern(is, pattern);

//  boost::process::ipstream is;
  auto process = launchCommand(
              cmd/*,
              boost::process::std_out > is,
              boost::process::std_in < boost::process::null*/
              );

  insight::assertion(
              process->running(),
              "could not start background process");


  return std::make_shared<BackgroundJob>(*this, std::move(process));
}




void WSLLinuxServer::runRsync
(
    const std::vector<std::string>& args,
    std::function<void(int,const std::string&)> pf
)
{
  CurrentExceptionContext ex("runRsync, args: "+boost::join(args, " "));

  assertRunning();

  boost::process::ipstream is;

  std::string joinedArgs="rsync --info=progress2";
  for (const auto& a: args)
  {
    joinedArgs+=" "+escapeShellSymbols(a);
  }
  auto ca = config().commandAndArgs(joinedArgs);

  RSyncOutputAnalyzer rpa(pf);
  auto job = std::make_shared<Job>(ca);
  job->ios_run_with_interruption(&rpa);
  job->wait();

}




boost::filesystem::path convertWindowsPathToWSLPath(const boost::filesystem::path& wp)
{
  auto wpg = boost::filesystem::absolute(wp).generic_path();
  auto drive = wpg.root_name().string().substr(0, 1);
  boost::algorithm::to_lower(drive);
  return (boost::filesystem::path("/mnt") / drive / wpg.relative_path()).generic_path();
}




void WSLLinuxServer::putFile
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
       convertWindowsPathToWSLPath(localFilePath).string(),
       remoteFilePath.string()
      };

  runRsync(args, pf);
}




void WSLLinuxServer::syncToRemote
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

//         "--exclude", "processor*",
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

    args.push_back(convertWindowsPathToWSLPath(localDir).string()+"/");
    args.push_back(remoteDir.string());

    runRsync(args, pf);
}




void WSLLinuxServer::syncToLocal
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
//      "--exclude", "processor*",
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

    args.push_back(remoteDir.string()+"/");
    args.push_back(convertWindowsPathToWSLPath(localDir).string());

    runRsync(args, pf);
}




string WSLLinuxServer::defaultRepositoryURL(const ToolkitVersion &tv)
{

    if (tv.branch()=="master")
    {
        return "http://downloads.silentdynamics.de/ubuntu";
    }
    else if (tv.branch()=="next-release")
    {
        return "http://downloads.silentdynamics.de/ubuntu_dev";
    }
    else
    {
        std::string bn = tv.branch();
        boost::regex re("^customer-(.*)$");
        boost::smatch group;
        if (boost::regex_search(bn, group, re))
        {
           return str(format
                      ("https://rostock.kroegeronline.net/customers/%s")
                        % group[1] );
        }
        else
        {
            throw insight::Exception("(could not be determined, please contact support)");
        }
    }
}




string WSLLinuxServer::installationPackageName(const ToolkitVersion &tv)
{
    std::string packageName="insightcae";

    if ( tv.branch()=="next-release"
         || tv.branch()=="master" )
    {
      packageName="insightcae-ce";
    }

    return packageName;
}



string WSLLinuxServer::defaultWSLDistributionName(const ToolkitVersion &tv)
{
    insight::CurrentExceptionContext ex("building default WSL distribution label");

    std::string wslname("insightcae-ubuntu-1804");

    boost::regex re("^customer-(.*)$");
    boost::smatch group;
    if (boost::regex_search(tv.branch(), group, re))
    {
        wslname += "-"+group[1];
    }

    insight::dbg()<<"default WSL name: "<<wslname<<std::endl;
    return wslname;
}




std::vector<std::string> WSLLinuxServer::listWSLDistributions()
{
    std::vector<std::string> distros;

#ifdef WIN32
    boost::process::wipstream out;

    int ret = boost::process::system(
                WSLcommand(),
                boost::process::args({"--list", "--quiet"}),
                boost::process::std_out > out,
                boost::process::std_err > stderr,
                boost::process::std_in < boost::process::null
                );

    if (ret==0)
    {
        using convert_type = std::codecvt_utf8<wchar_t>;
        std::wstring_convert<convert_type, wchar_t> converter;
        wstring wline;
        while (getline(out, wline))
        {
            std::string line=converter.to_bytes( wline );
            boost::trim(line);
            if (!line.empty())
            {
                insight::dbg()<<line<<std::endl;
                distros.push_back(line);
            }
        }
    }
    else
    {
        // WSL not activated?
    }
#endif

    return distros;
}




std::mutex g_child_mutex;

ToolkitVersion WSLLinuxServer::checkInstalledVersion()
{
  boost::process::ipstream out;

  std::string cmd="/usr/bin/insight_version.sh";

  int ret = executeCommand(
      cmd, false,
      boost::process::std_out > out,
      boost::process::std_err > stderr,
      boost::process::std_in < boost::process::null
      );


  dbg()<<"ret="<<ret<<std::endl;
  if (ret==0)
  {
    boost::regex pat("^  Installed: (.*)\\.(.*)\\.(.*)-(.*)~(.*)$");
    std::string line;
    while (getline(out, line))
    {
      dbg()<<line<<std::endl;
      boost::smatch m;
      if (boost::regex_search(line, m, pat))
      {
        dbg()<<m[1]<<" "<<m[2]<<" "<<m[3]<<" "<<m[4]<<" "<<m[5]<<std::endl;
        int majorVersion = lexical_cast<int>(m[1]);
        int minorVersion = lexical_cast<int>(m[2]);
        int patchVersion = lexical_cast<int>(m[3]);
        std::string commit = m[4];
        return ToolkitVersion(majorVersion, minorVersion, patchVersion, commit, "");
      }
    }
    throw insight::Exception("Could not parse version information.\nMaybe package is not installed?");
  }
  else
  {
    throw insight::Exception("Could not execute command for version check (\""+cmd+"\")");
  }
}




void WSLLinuxServer::updateInstallation(
        std::function<void(const std::string&)> logCallback )
{

    boost::process::ipstream os;

    auto process =  launchCommand(
          "sudo /usr/bin/insight_update.sh",
          boost::process::std_out > os,
          boost::process::std_err > stderr,
          boost::process::std_in < boost::process::null
          );

    if (!process->running())
    {
     throw insight::Exception("could not launch WSL update command");
    }

    while (process->running())
    {
      std::string line;
      if (getline(os, line))
      {
          logCallback(line);
      }
    }
    process->wait();
    if (process->exit_code()!=0)
    {
     throw insight::Exception("WSL update command failed");
    }
}




} // namespace insight
