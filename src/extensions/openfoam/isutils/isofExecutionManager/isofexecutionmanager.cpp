/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "base/boost_include.h"
#include "base/linearalgebra.h"
#include "base/analysis.h"
#include "base/mountremote.h"
#include "openfoam/openfoamtools.h"

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>

#include "insightcaeapplication.h"
#include "mainwindow.h"
#include "remotedirselector.h"

using namespace std;
using namespace insight;
using namespace boost;
namespace bf = boost::filesystem;

void printProgress(int progress_percent, const string & msg)
{
    double progress = double(progress_percent)/100.;
    int barWidth = 20, totalWidth=80;

    std::cout << "[";
    int pos =  barWidth * progress;
    for (int i = 0; i < barWidth; ++i)
    {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "]";
    std::cout << str(format(" %3d%% ") % progress_percent);

    std::string fixedMsg(
                std::max(0, totalWidth-barWidth-2-5),
                ' ' );
    for (int i=0; i<fixedMsg.size(); ++i)
    {
        if (i<msg.size())  fixedMsg[i]=msg[i];
    }
    std::cout<<fixedMsg;

    std::cout<<"\r";
    std::cout.flush();
}


int main(int argc, char *argv[])
{

  bool anything_done=false;

  //    insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;

  namespace po = boost::program_options;

  typedef std::vector<string> StringList;

  StringList cmds, icmds, skip_dirs, remoteServerFilter;
  std::string remoteAnalysisFileName;

  // Declare the supported options.
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help,h", "produce help message")

      //("ofe,o", po::value<std::string>(), "use specified OpenFOAM environment instead of detected")
      ("meta-file,m", po::value<std::string>(), "use the specified remote execution config file instead of \"meta.foam\"")
      ("case-dir,d", po::value<std::string>(), "case location")
      ("remote-dir-for-mapping", po::value<std::string>(), "return remote directory name of another case to be used as result mapping source. An error will occur, if the remote dir does not exists or is on another computer than the remote of the current case.")
      ("create-remote-temp,T", po::value<std::string>(), "create a unique remote directory name on the specified server and store in meta file. This will be skipped, if config exists already, except if -f is given.")
      ("force-create-remote-temp,f", "force creation, if config exists already.")
      ("sync-remote,r", "sync current case to remote location")
      ("sync-local,l", "sync from remote location to current case")
      ("sync-local-repeat", po::value<int>(), "sync from remote location to current case, restart transfer periodically after given number of seconds")
      ("reconst-new", "reconstruct time directories which are yet unreconstructed")
      ("skip-timesteps,t", "exclude time steps while syncing to local directory\nBeware: during a subsequent sync-to-remote, the skipped time steps will be deleted!")
      ("skip-dir,s", po::value<StringList>(&skip_dirs), "exclude local directory during sync to remote")
      ("include-processor-dirs,p", "if this flag is set, processor directories will be tranferred as well")
      ("command,q", po::value<StringList>(&cmds), "add this command to remote execution queue (will be executed after the previous command has finished successfully)")
      ("immediate-command,i", po::value<StringList>(&icmds), "add this command to remote execution queue (execute in parallel without waiting for the previous command)")
      ("wait,w", "wait for command queue completion")
      ("wait-last,W", "wait for the last added command to finish")
      ("cancel,c", "cancel remote commands (remove all from queue)")
      ("clean,x", "remove the remote case directory from server")
      ("list-remote,D", "list remote directory contents")
      ("locate-remote-configs,L", "locate and list existing remote configurations")
      ("filter-remote-server,S", po::value<StringList>(&remoteServerFilter), "filter remote configuration list by server")
      ("bwlimit", po::value<int>()->default_value(-1), "transfer bandwidth limitin kB/s; -1 means no limit")
      ("launch-analysis", po::value<std::string>(&remoteAnalysisFileName), "launch \"analyze\" on the specified file.")
#ifndef WIN32
      ("mount-remote,M", "mount the remote directory locally using sshfs (needs to be installed)")
      ("unmount-remote,U", "unmount the remote directory")
#endif
      ;

  po::positional_options_description p;
  p.add("command", -1);

  auto displayHelp = [&]{
    std::ostream &os = std::cout;

    os << "Usage:\n"
          " " << boost::filesystem::path(argv[0]).filename().string() << " [options] " << p.name_for_position(0) << "\n"
       << desc;
  };

  po::variables_map vm;
  try
  {
    po::store(po::command_line_parser(argc, argv).
              options(desc).positional(p).run(), vm);
    po::notify(vm);
  }
  catch (const po::error& e)
  {
    std::cerr << std::endl << "Could not parse command line: " << e.what() << std::endl<<std::endl;
    displayHelp();
    exit(-1);
  }

  if (vm.count("help"))
  {
    displayHelp();
    exit(0);
  }

  boost::filesystem::path location=".";
  if (vm.count("case-dir"))
  {
    location = vm["case-dir"].as<std::string>();
  }

  bool include_processor=false;
  if (vm.count("include-processor-dirs")>0)
  {
    include_processor=true;
  }

  bfs_path mf="";
  if (vm.count("meta-file"))
  {
    mf=vm["meta-file"].as<std::string>();
  }

  if (vm.count("locate-remote-configs"))
  {
      auto mfn=insight::RemoteExecutionConfig::defaultConfigFileName();
      if (!mf.empty()) mfn=mf.filename();
      auto j = Job::forkExternalProcess(
                  "locate",
                  {"-e", mfn.string()});
      std::vector<std::string> files;
      j->runAndTransferOutput(&files, nullptr, false, true);
      if (files.size())
      {
          std::cout<<"# local directory \t server label \t remote directory"<<std::endl;
      }

      for (const auto& fp: files)
      {
          auto localDir = boost::filesystem::path(fp).parent_path();
          insight::RemoteLocation rl(fp, true);

          auto serverLabel = rl.serverLabel();

          bool filtered = false;

          if ( remoteServerFilter.size()
               &&
               remoteServerFilter.end()==std::find(
                   remoteServerFilter.begin(), remoteServerFilter.end(), serverLabel) )
              filtered=true;

          if (!filtered)
            std::cout<<localDir.string()<<" \t "<<serverLabel<<" \t "<<rl.remoteDir().string()<<std::endl;
      }
      anything_done=true;
  }

  if (vm.count("create-remote-temp"))
  {
    bf::path meta=mf;
    if (meta.empty())
    {
      meta=insight::RemoteExecutionConfig::defaultConfigFile(location);
    }

    if (bf::exists(meta)&&(!vm.count("force-create-remote-temp")))
    {
      insight::Warning("remote config exists already: creation of temporary remote directory will be skipped.");
    }
    else
    {

      string server=vm["create-remote-temp"].as<std::string>();
      auto i = insight::remoteServers.findServer(server);

      insight::RemoteExecutionConfig rec(i, location); // creates config file
      rec.initialize();
      rec.writeConfig(meta);

      anything_done=true;
    }
  }

  try
  {

    std::unique_ptr<insight::RemoteExecutionConfig> re;
    try
    {
      re.reset(new insight::RemoteExecutionConfig(location, mf));
    }
    catch (...) {}
    boost::filesystem::path local_mp = location / "mnt_remote" / "default";

    if (re && re->isActive())
    {

      re->server()
              ->setTransferBandWidthLimit(
                  vm["bwlimit"].as<int>() );

      if(vm.count("list-remote"))
      {
        auto files=re->remoteLS();
        for (const auto&f: files)
        {
          std::cout<<f<<std::endl;
        }
        anything_done=true;
      }

#ifndef WIN32
      if(vm.count("mount-remote"))
      {
        if (!boost::filesystem::exists(local_mp))
          boost::filesystem::create_directories(local_mp);

        MountRemote m(local_mp, *re, true);

        anything_done=true;
      }
#endif

      if(vm.count("cancel"))
      {
        re->cancelRemoteCommands();
        anything_done=true;
      }

      if (vm.count("sync-remote"))
      {
        re->syncToRemote(
                    include_processor,
                    skip_dirs,
                    printProgress );
        std::cout<<endl;

        anything_done=true;
      }

      if (vm.count("command"))
      {
        for (const auto& c: cmds)
        {
          re->queueRemoteCommand(c);
          anything_done=true;
        }
      }

      if (vm.count("immediate-command"))
      {
        for (const auto& c: icmds)
        {
          re->queueRemoteCommand(c, false);
          anything_done=true;
        }
      }

      if (vm.count("launch-analysis"))
      {
          re->queueRemoteCommand("analyze "+remoteAnalysisFileName);
          anything_done=true;
      }

      if (vm.count("wait"))
      {
        re->waitRemoteQueueFinished();
        anything_done=true;
      }

      if (vm.count("wait-last"))
      {
        re->waitLastCommandFinished();
        anything_done=true;
      }

      if(vm.count("sync-local"))
      {
        re->syncToLocal(
                    include_processor,
                    (vm.count("skip-timesteps")>0),
                    skip_dirs,
                    printProgress );
        std::cout<<endl;

        anything_done=true;
      }

      auto reconstNew = [&]()
      {
          OpenFOAMCase cm;
          ParallelTimeDirectories ptd(cm, location);
          auto rtds = ptd.newParallelTimes(true);
          rtds.erase(boost::filesystem::path("0"));

          std::vector<std::string> tdbns;
          std::transform(rtds.begin(), rtds.end(), std::back_inserter(tdbns),
                         [&](const decltype(rtds)::value_type& path)
                         {
                             return path.string();
                         }
                         );

          if (rtds.size())
          {
              cm.executeCommand(
                  location,
                  "reconstructPar",
                  { "-time", boost::join(tdbns, ",") }
                  );
          }
      };

      if(vm.count("sync-local-repeat"))
      {
        int secs=vm["sync-local-repeat"].as<int>();

        while (true)
        {
            re->syncToLocal(
                include_processor,
                (vm.count("skip-timesteps")>0),
                skip_dirs,
                printProgress );
            std::cout<<endl;

            if (vm.count("reconst-new"))
            {
                std::cout<<"Reconstructing newly appeared time directories..."<<std::endl;
                reconstNew();
            }

            std::cout<<"Waiting "<<secs<<" seconds before restarting transfer..."<<std::endl;
            sleep(secs);
        }

        anything_done=true;
      }

      if (vm.count("reconst-new"))
      {
        reconstNew();
        anything_done=true;
      }

#ifndef WIN32
      if(vm.count("unmount-remote"))
      {
        if (!boost::filesystem::exists(local_mp))
          throw insight::Exception("mount point "+local_mp.string()+" does not exist!");
        else {
          MountRemote m(local_mp, *re, false, true);
        }
        anything_done=true;
      }
#endif


      if(vm.count("clean"))
      {
        re->cleanup(true);
        return 0; // configuration is invalidated, exit here
      }

      if (vm.count("remote-dir-for-mapping"))
      {
        boost::filesystem::path other_case_path( vm["remote-dir-for-mapping"].as<std::string>() );
        RemoteExecutionConfig orc(other_case_path);

        if (orc.server() != re->server())
          throw insight::Exception("Remote directory of case is on a different server than the current case");

        if (!orc.remoteDirExists())
          throw insight::Exception("Remote directory of case in "+other_case_path.string()+" does not exists!");

        std::cout<<orc.remoteDir().string()<<std::endl;
        anything_done=true;
      }
    }
  }
  catch (const std::exception& e)
  {
    insight::printException(e);
    exit(-1);
  }

  if (!anything_done)
  {
    InsightCAEApplication app(argc, argv, "isofExecutionManager");
    MainWindow w(location);
    w.show();
    return app.exec();
  }

}
