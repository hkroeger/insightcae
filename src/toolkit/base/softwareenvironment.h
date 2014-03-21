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


#ifndef SOFTWAREENVIRONMENT_H
#define SOFTWAREENVIRONMENT_H

#include <vector>
#include <string>
#include <iostream>

#include "pstreams/pstream.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"

namespace insight
{

class SoftwareEnvironment
{
  
  std::string executionMachine_;

public:
    SoftwareEnvironment();
    SoftwareEnvironment(const SoftwareEnvironment& other);
    virtual ~SoftwareEnvironment();
    
    inline void setExecutionMachine(const std::string& executionMachine) { executionMachine_=executionMachine; }
    inline const std::string& executionMachine() const { return executionMachine_; }
    
    virtual int version() const =0;
    
    virtual int executeCommand
    (  
      const std::string& cmd, 
      std::vector<std::string> argv = std::vector<std::string>(),
      std::vector<std::string>* output = NULL,
      std::string *ovr_machine=NULL
    ) const;
    
    template<class stream>
    void forkCommand
    (
      /*redi::ip*/ stream& p_in,      
      const std::string& cmd, 
      std::vector<std::string> argv = std::vector<std::string>(),
      std::string *ovr_machine=NULL
    ) const
    {
      
      std::string machine=executionMachine_;
      if (ovr_machine) machine=*ovr_machine;
      
      argv.insert(argv.begin(), cmd);
      
      if (machine=="")
      {
	argv.insert(argv.begin(), "-c");
	argv.insert(argv.begin(), "bash");
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
      
      std::cout<<"argv=( ";
      int k=0;
      BOOST_FOREACH(const std::string& a, argv) std::cout<<(++k)<<":"<<a<<" ";
      std::cout<<")"<<std::endl;
      
      if (argv.size()>1)
      {
	p_in.open(argv[0], argv);
      }
      else
      {
	p_in.open(argv[0]);
      }
      
      std::cout<<"Executing "<<p_in.command()<<std::endl;
    }
    
};

}

#endif // SOFTWAREENVIRONMENT_H
