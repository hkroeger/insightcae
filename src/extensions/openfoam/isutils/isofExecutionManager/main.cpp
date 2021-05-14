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


int main(int argc, char *argv[])
{

  bool anything_done=false;

  //    insight::UnhandledExceptionHandling ueh;
  insight::GSLExceptionHandling gsl_errtreatment;

  namespace po = boost::program_options;

  typedef std::vector<string> StringList;

  StringList cmds, icmds, skip_dirs;

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
      ("mount-remote,M", "mount the remote directory locally using sshfs (needs to be installed)")
      ("unmount-remote,U", "unmount the remote directory")
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

      insight::RemoteExecutionConfig rec(i.second, location); // creates config file

      anything_done=true;
    }
  }

  try
  {

    insight::RemoteExecutionConfig re(location, mf);
    boost::filesystem::path local_mp = location / "mnt_remote" / "default";

    if (re.isValid())
    {

      if(vm.count("list-remote"))
      {
        auto files=re.remoteLS();
        for (const auto&f: files)
        {
          std::cout<<f<<std::endl;
        }
        anything_done=true;
      }

      if(vm.count("mount-remote"))
      {
        if (!boost::filesystem::exists(local_mp))
          boost::filesystem::create_directories(local_mp);

        MountRemote m(local_mp, re.server(), re.remoteDir(), true);

        anything_done=true;
      }

      if(vm.count("cancel"))
      {
        re.cancelRemoteCommands();
        anything_done=true;
      }

      if (vm.count("sync-remote"))
      {
        re.syncToRemote(skip_dirs);
        anything_done=true;
      }

      if (vm.count("command"))
      {
        for (const auto& c: cmds)
        {
          re.queueRemoteCommand(c);
          anything_done=true;
        }
      }

      if (vm.count("immediate-command"))
      {
        for (const auto& c: icmds)
        {
          re.queueRemoteCommand(c, false);
          anything_done=true;
        }
      }

      if (vm.count("wait"))
      {
        re.waitRemoteQueueFinished();
        anything_done=true;
      }

      if (vm.count("wait-last"))
      {
        re.waitLastCommandFinished();
        anything_done=true;
      }

      if(vm.count("sync-local"))
      {
        re.syncToLocal( (vm.count("skip-timesteps")>0), skip_dirs );
        anything_done=true;
      }

      if(vm.count("unmount-remote"))
      {
        if (!boost::filesystem::exists(local_mp))
          throw insight::Exception("mount point "+local_mp.string()+" does not exist!");
        else {
          MountRemote m(local_mp, re.server(), re.remoteDir(), false, true);
        }
        anything_done=true;
      }


      if(vm.count("clean"))
      {
        re.cleanup();
        return 0; // configuration is invalidated, exit here
      }

      if (vm.count("remote-dir-for-mapping"))
      {
        boost::filesystem::path other_case_path( vm["remote-dir-for-mapping"].as<std::string>() );
        RemoteExecutionConfig orc(other_case_path);

        if (orc.server() != re.server())
          throw insight::Exception("Remote directory of case in "+other_case_path.string()+" is on server "+orc.server()+" which is different than the server of the current case "+re.server());

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
