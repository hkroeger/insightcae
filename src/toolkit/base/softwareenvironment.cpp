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


#include "base/softwareenvironment.h"
#include <boost/asio.hpp>
#include <boost/process/async.hpp>


using namespace std;

namespace insight
{





SoftwareEnvironment::Job::Job()
  : out(ios), err(ios)
{
}




void SoftwareEnvironment::Job::runAndTransferOutput
(
    std::vector<std::string>* stdout,
    std::vector<std::string>* stderr
)
{


  std::function<void()> read_start_out = [&]() {
    boost::asio::async_read_until(
     out, buf_out, "\n",
     [&](const boost::system::error_code &error, std::size_t /*size*/)
     {
      if (!error)
      {
        // read a line
       std::string line;
       std::istream is(&buf_out);
       getline(is, line);

       // mirror to console
       std::cout<<line<<std::endl;
       if (stdout) stdout->push_back(line);

       // restart read
       read_start_out();
      }
     }
    );
  };


  std::function<void()> read_start_err = [&]() {
    boost::asio::async_read_until(
     err, buf_err, "\n",
     [&](const boost::system::error_code &error, std::size_t /*size*/)
     {
      if (!error)
      {
        // read a line
       std::string line;
       std::istream is(&buf_err);
       getline(is, line);

       // mirror to console
       std::cout<<"[E] "<<line<<std::endl;
       if (stderr)
       {
         stderr->push_back(line);
       }
       else
       {
         if (stdout)
           stdout->push_back("[E] "+line);
       }

       read_start_err();
      }
     }
    );
  };

  read_start_out();
  read_start_err();

  ios.run();
  process->wait(); // exit code is not set correctly, if this is skipped
}




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




void SoftwareEnvironment::executeCommand
(
  const std::string& cmd, 
  std::vector<std::string> argv,
  std::vector<std::string>* output,
  std::string *ovr_machine
) const
{
  
  JobPtr job = forkCommand(cmd, argv, ovr_machine);

  job->runAndTransferOutput(output);

  if (job->process->exit_code()!=0)
  {
    std::ostringstream os;
    os << cmd;
    for ( const std::string& s: argv )
    {
      os<<" "<<s;
    }
    throw insight::Exception("SoftwareEnvironment::executeCommand(): command failed with nonzero return code.\n(Command was \""+os.str()+"\"");
  }
  
  //return p_in.rdbuf()->status();
}




SoftwareEnvironment::JobPtr SoftwareEnvironment::forkCommand
(
  const std::string& cmd,
  std::vector<std::string> argv,
  std::string *ovr_machine
) const
{

  std::string machine=executionMachine_;
  if (ovr_machine) machine=*ovr_machine;

  argv.insert(argv.begin(), cmd);

  if (machine=="")
    {
        argv.insert(argv.begin(), "-lc");
        argv.insert(argv.begin(), "bash");
        // keep only a selected set of environment variables
        std::vector<std::string> keepvars = boost::assign::list_of("DISPLAY")("HOME")("USER")("SHELL")("INSIGHT_BINDIR")("INSIGHT_LIBDIR")("INSIGHT_OFES");
        for (const std::string& varname: keepvars)
        {
            if (char* varvalue=getenv(varname.c_str()))
            {
                // shellcmd+="export "+varname+"=\""+std::string(varvalue)+"\";";
                argv.insert(argv.begin(), varname+"="+std::string(varvalue));
            }
        }
        argv.insert(argv.begin(), "-i");
        argv.insert(argv.begin(), "env");
    }
  else if (boost::starts_with(machine, "qrsh-wrap"))
  {
    //argv.insert(argv.begin(), "n");
    //argv.insert(argv.begin(), "-now");
    argv.insert(argv.begin(), "qrsh-wrap");
  }
  else
  {
    argv.insert(argv.begin(), machine);
    argv.insert(argv.begin(), "ssh");
  }

  std::ostringstream dbgs;
  for (const std::string& a: argv)
    dbgs<<a<<" ";
  std::cout<<dbgs.str()<<std::endl;

  JobPtr job(new Job);

  namespace bp = boost::process;
  std::vector<std::string> args(argv.begin()+1, argv.end());

//std::cout<<argv[0]<<std::endl;
//for(const auto& arg: args)
//  std::cout<<arg<<std::endl;

  job->process.reset(
        new bp::child(
           bp::search_path(argv[0]),
           bp::args( args ),
           bp::std_in < job->in,
           bp::std_out > job->out,
           bp::std_err > job->err
          )
        );

  if (!job->process->running())
  {
    throw insight::Exception("SoftwareEnvironment::forkCommand(): Failed to launch subprocess!\n(Command was \""+dbgs.str()+"\")");
  }

  std::cout<<"Executing "<<dbgs.str()<<std::endl;

  return job;
}

}
