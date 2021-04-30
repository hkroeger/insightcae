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


// #include "boost/foreach.hpp"
// #include "boost/algorithm/string.hpp"
#include "base/boost_include.h"
#include "boost/process.hpp"
#include "boost/asio/io_service.hpp"

#include "base/exception.h"

namespace insight
{


class SoftwareEnvironment
{
public:

  struct Job
  {
    boost::asio::io_service ios;
    boost::process::opstream in;
    boost::process::async_pipe out, err;
    boost::asio::streambuf buf_out, buf_err;

    std::shared_ptr<boost::process::child> process;

    Job();

    void runAndTransferOutput
    (
        std::vector<std::string>* pstdout = nullptr,
        std::vector<std::string>* pstderr = nullptr
    );

    void ios_run_with_interruption();
  };

 typedef std::shared_ptr<Job> JobPtr;

private:
  
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
      std::vector<std::string>* output = nullptr,
      std::string *ovr_machine = nullptr
    ) const;
    
    JobPtr forkCommand
    (
        const std::string& cmd,
        std::vector<std::string> argv = std::vector<std::string>(),
        std::string *ovr_machine = nullptr
    ) const;
    
};

}

#endif // SOFTWAREENVIRONMENT_H
