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

#include <iostream>

using namespace std;

namespace insight
{

SoftwareEnvironment::SoftwareEnvironment()
{

}

SoftwareEnvironment::SoftwareEnvironment(const SoftwareEnvironment& other)
{

}

SoftwareEnvironment::~SoftwareEnvironment()
{

}

int SoftwareEnvironment::executeCommand
(
  const std::string& cmd, 
  std::vector<std::string> argv,
  std::vector<std::string>* output
) const
{
  redi::ipstream p_in;
  
  forkCommand(p_in, cmd, argv);
  
  std::string line;
  while (std::getline(p_in, line))
  {
    cout<<">> "<<line<<endl;
    if (output) output->push_back(line);
  }
  p_in.close();

  return p_in.rdbuf()->status();
}

void SoftwareEnvironment::forkCommand
(
  redi::ipstream& p_in,
  const std::string& cmd, 
  std::vector<std::string> argv
) const
{
  if (argv.size()>0)
  {
    argv.insert(argv.begin(), cmd);
    p_in.open(cmd, argv);
  }
  else
  {
    p_in.open(cmd);
  }
  
  cout<<"Executing "<<p_in.command()<<endl;
}

}
