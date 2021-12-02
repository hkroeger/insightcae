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


#include "base/tools.h"
#include "base/softwareenvironment.h"

#include <boost/asio.hpp>
#include <boost/process/async.hpp>
#include <boost/asio/steady_timer.hpp>

using namespace std;
namespace fs=boost::filesystem;

namespace insight
{



SoftwareEnvironment::SoftwareEnvironment()
: executionMachine_("")
{

}




SoftwareEnvironment::SoftwareEnvironment(const SoftwareEnvironment& other)
: executionMachine_(other.executionMachine_)
{
}




SoftwareEnvironment::~SoftwareEnvironment()
{

}




int SoftwareEnvironment::version() const
{
  return -1;
}

fs::path SoftwareEnvironment::which(const string &command) const
{
  insight::CurrentExceptionContext ex("determining full path to "+command);
  std::vector<std::string> result;

  executeCommand("which", { command }, &result);

  if (result.size()!=1)
    throw insight::Exception("Command executable "+command+" not found!");
  return fs::path(result[0]);
}




void SoftwareEnvironment::executeCommand
(
  const std::string& cmd, 
  std::vector<std::string> argv,
  std::vector<std::string>* output,
  std::string *ovr_machine
) const
{
  std::vector<std::string> cmds;
  boost::split(cmds, cmd, boost::is_any_of(";"), boost::token_compress_on);
  std::string finalcmd=cmds.back();

  CurrentExceptionContext ex(
        "executing command \""+finalcmd+"\""
        + (argv.size()>0 ? " with arguments:\n"+boost::join(argv, "\n"):"")
        );

  JobPtr job = forkCommand(cmd, argv, ovr_machine);

  std::vector<std::string> errout;
  job->runAndTransferOutput(output, &errout);

  auto retcode = job->process().exit_code();
  if (retcode!=0)
  {
    throw insight::Exception(
          boost::str(boost::format(
             "Execution of external application \"%s\" failed with return code %d!\n")
              % finalcmd % retcode)
          + ( errout.size()>0 ?
               ("Error output was:\n\n" + boost::join(errout, "\n")+"\n")
               :
               "There was no error output."
             )
          );
  }
  
  //return p_in.rdbuf()->status();
}




JobPtr SoftwareEnvironment::forkCommand
(
  const std::string& cmd_exe,
  std::vector<std::string> cmd_argv,
  std::string *ovr_machine
) const
{
  CurrentExceptionContext ex(
        "launching command \""+cmd_exe+"\" as subprocess"
        + (cmd_argv.size()>0 ? " with arguments:\n"+boost::join(cmd_argv, "\n") : "")
        );

  std::string machine=executionMachine_;
  if (ovr_machine) machine=*ovr_machine;

  std::vector<std::string> argv;

  if (machine=="")
  {
      // set up clean environment for InsightCAE
      argv.insert(argv.end(), {
                    "env", "-i"
                  });
      // keep only a selected set of environment variables
      std::vector<std::string> keepvars = {
        "DISPLAY", "HOME", "USER", "SHELL",
        //"INSIGHT_INSTDIR", "INSIGHT_BINDIR", "INSIGHT_LIBDIR", "INSIGHT_LIBDIRS", "INSIGHT_OFES",
        "LANG", "LC_ALL", // req. for python/Code_Aster
        "PYTHONPATH" //, "INSIGHT_THIRDPARTY_DIR"
      };
      for (const std::string& varname: keepvars)
      {
          if (char* varvalue=getenv(varname.c_str()))
          {
              argv.push_back(varname+"="+std::string(varvalue));
          }
      }
      argv.insert(argv.end(), {
                    "bash", "-lc"
                  });
  }
  else if (boost::starts_with(machine, "qrsh-wrap"))
  {
    //argv.insert(argv.begin(), "n");
    //argv.insert(argv.begin(), "-now");
    argv.insert(argv.end(), "qrsh-wrap");
  }
  else
  {
    argv.insert(argv.end(), { "ssh", machine} );
  }

  // wrap into single command string
  std::string cmd = cmd_exe;
  if (char* cfgpath=getenv("INSIGHT_CONFIGSCRIPT"))
  {
      cmd = "source "+std::string(cfgpath)+";" + cmd;
  }
  else
  {
    insight::Warning("There is no INSIGHT_CONFIGSCRIPT variable defined! Please check environment configuration.");
  }

  for (const auto& a: cmd_argv)
  {
    cmd+=" \""+escapeShellSymbols(a)+"\"";
  }
  argv.insert(argv.end(), cmd);

  for (const std::string& a: argv)
    std::cout<<a<<" ";
  std::cout<<std::endl;

  std::vector<std::string> args(argv.begin()+1, argv.end());

  return Job::forkExternalProcess(argv[0], args);
}

}
