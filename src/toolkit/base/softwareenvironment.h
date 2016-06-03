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


#ifndef SOFTWAREENVIRONMENT_H
#define SOFTWAREENVIRONMENT_H

#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "pstreams/pstream.h"

#include "boost/foreach.hpp"
#include "boost/algorithm/string.hpp"

#include "base/exception.h"

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
    
    virtual int version() const;
    
    virtual void executeCommand
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
      
      std::ostringstream dbgs;
      BOOST_FOREACH(const std::string& a, argv) 
	dbgs<<a<<" ";
	
      std::cout<<dbgs.str()<<std::endl;
      
      if (argv.size()>1)
      {
	p_in.open(argv[0], argv);
      }
      else
      {
	p_in.open(argv[0]);
      }
      
      if (!p_in.is_open())
      {
	throw insight::Exception("SoftwareEnvironment::forkCommand(): Failed to launch subprocess!\n(Command was \""+dbgs.str()+"\")");
      }
      
      std::cout<<"Executing "<<p_in.command()<<std::endl;
    }
    
};

}

#endif // SOFTWAREENVIRONMENT_H
