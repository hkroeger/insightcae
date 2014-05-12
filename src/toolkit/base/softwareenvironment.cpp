/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "base/softwareenvironment.h"

using namespace std;

namespace insight
{

SoftwareEnvironment::SoftwareEnvironment()
: executionMachine_("")
{

}

SoftwareEnvironment::SoftwareEnvironment(const SoftwareEnvironment& other)
{

}

SoftwareEnvironment::~SoftwareEnvironment()
{

}

void SoftwareEnvironment::executeCommand
(
  const std::string& cmd, 
  std::vector<std::string> argv,
  std::vector<std::string>* output,
  std::string *ovr_machine
) const
{
  redi::ipstream p_in;
  
  forkCommand(p_in, cmd, argv, ovr_machine);
  
  std::string line;
  while (std::getline(p_in, line))
  {
    cout<<">> "<<line<<endl;
    if (output) output->push_back(line);
  }
  p_in.close();
  
  if (p_in.rdbuf()->status()!=0)
    throw insight::Exception("SoftwareEnvironment::executeCommand(): command failed with nonzero return code.");

  //return p_in.rdbuf()->status();
}

/*
void SoftwareEnvironment::forkCommand
(
  redi::ipstream& p_in,
  const std::string& cmd, 
  std::vector<std::string> argv
) const
{
  
  argv.insert(argv.begin(), cmd);
  
  if (executionMachine_=="")
  {
    argv.insert(argv.begin(), "-c");
    argv.insert(argv.begin(), "bash");
  }
  else if (boost::starts_with(executionMachine_, "qrsh:"))
  {
    argv.insert(argv.begin(), "qrsh");
  }
  else
  {
    argv.insert(argv.begin(), executionMachine_);
    argv.insert(argv.begin(), "ssh");
  }
  
  cout<<"argv=( ";
  int k=0;
  BOOST_FOREACH(const std::string& a, argv) cout<<(++k)<<":"<<a<<" ";
  cout<<")"<<endl;
  
  if (argv.size()>1)
  {
    p_in.open(argv[0], argv);
  }
  else
  {
    p_in.open(argv[0]);
  }
  
  cout<<"Executing "<<p_in.command()<<endl;
}
*/
}
