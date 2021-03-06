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

#include "codeasterrun.h"
#include "caexportfile.h"

#include "base/outputanalyzer.h"
#include "base/filewatcher.h"

#include "boost/process.hpp"

#include <memory>
#include <boost/asio.hpp>
#include <boost/process/async.hpp>

using namespace std;
using namespace boost;
namespace fs=boost::filesystem;

namespace insight
{

CAEnvironment::CAEnvironment(const boost::filesystem::path& asrun_cmd)
  : asrun_cmd_(asrun_cmd)
{
  if (!fs::exists(asrun_cmd_))
    asrun_cmd_=which(asrun_cmd_.string());
}

int CAEnvironment::version() const
{
  return 11;
}

const boost::filesystem::path& CAEnvironment::asrun_cmd() const
{
  return asrun_cmd_;
}

JobPtr CAEnvironment::forkCase
(
  const boost::filesystem::path& exportfile
) const
{
  boost::filesystem::path dir=exportfile.parent_path();

  return forkCommand(
        "cd "+dir.string()+"; "+asrun_cmd().string(),
        {"--run", exportfile.string()}
        );

}



void CAEnvironment::runSolver(const filesystem::path &exportfile, OutputAnalyzer &analyzer) const
{
 auto job = forkCase(exportfile);

 std::unique_ptr<FileWatcher> logFileWatcher;

 job->ios_run_with_interruption(
       [&](const std::string& line)
       {
         if ( boost::starts_with(
                line,
                "        -- CODE_ASTER -- VERSION :")
              && !logFileWatcher
              )
         {
           std::cout<<"Solver has started. launching log file parser."<<std::endl;

           CAExportFile cef(exportfile);

           auto wd = cef.workDir();
           if (wd.filename().string()=="global")
           {
             wd = wd.parent_path()/"proc.0";
           }
           auto logFile = wd/"fort.6";

           logFileWatcher.reset(
                 new FileWatcher(
                   logFile,
                   [&](const std::string& line)
                   {
                     std::cout<<line<<std::endl;
                     analyzer.update(line);
                   }, true ) );
         }
       },
       [&](const std::string& line)
       {
         // mirror to console
         cout<<"[E] "<<line<<endl; // mirror to console
       }
 );

 job->process->wait();


 if (job->process->exit_code()!=0)
     throw insight::Exception("CAEnvironment::runSolver(): solver run failed with nonzero exit code!");

}

}
