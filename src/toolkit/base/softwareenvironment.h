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

#include "pstreams/pstream.h"

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
      std::vector<std::string>* output = NULL
    ) const;
    
    virtual void forkCommand
    (
      redi::ipstream& p_in,      
      const std::string& cmd, 
      std::vector<std::string> argv = std::vector<std::string>()
    ) const;
    
};

}

#endif // SOFTWAREENVIRONMENT_H
