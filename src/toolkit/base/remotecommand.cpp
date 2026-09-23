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


#include "remotecommand.h"
#include "base/exception.h"
#include "base/externalprograms.h"
#include "base/stringconv.h"

#include "boost/process.hpp"
#include "boost/process/args.hpp"
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

using namespace std;
using namespace boost;

namespace insight
{


SSHCommand::SSHCommand(const std::string& hostName, const std::vector<std::string>& arguments)
  : hostName_(hostName), args_(arguments)
{
}

boost::filesystem::path SSHCommand::command() const
{
#if defined(WIN32)
    return ExternalPrograms::path("plink"); //boost::process::search_path("plink");
#else
    return ExternalPrograms::path("ssh"); //boost::process::search_path("ssh");
#endif
}

std::vector<std::string> SSHCommand::arguments() const
{
  std::vector<std::string> a(args_);
#if defined(WIN32)
  a.insert(a.begin(), { "-load", hostName_, "-no-antispoof", "-batch" });
#else
  a.insert(a.begin(), { hostName_ });
#endif
  return a;
}


RSYNCCommand::RSYNCCommand(const std::vector<std::string>& arguments)
  : args_(arguments)
{}

boost::filesystem::path RSYNCCommand::command() const
{
#if defined(WIN32)
    return boost::process::search_path("rsync.exe");
#else
    return boost::process::search_path("rsync");
#endif
}

std::vector<std::string> RSYNCCommand::arguments() const
{
  std::vector<std::string> a(args_);

#if defined(WIN32)
  auto cygnative = boost::process::search_path("cygnative.exe");
  auto plink = boost::process::search_path("plink.exe");
  a.insert(a.begin(),
           boost::str(boost::format("-e=\"%s %s -P %d\"")
                      % cygnative.string() % plink.string() % 22 )
  );
#else
  // keep args
#endif
  return a;
}


int findFreePort()
{
  using namespace boost::asio;
  using ip::tcp;

  io_service svc;
  tcp::acceptor a(svc);

  boost::system::error_code ec;
  a.open(tcp::v4(), ec) || a.bind({ tcp::v4(), 0 }, ec);

  if (ec == error::address_in_use)
  {
    throw insight::Exception("Could not find a free TCP/IP port on the local machine!");
  }
  else
  {
   return  a.local_endpoint().port(); //.address().to_string();
  }
}


int findRemoteFreePort(const std::string& SSHHostName)
{
  CurrentExceptionContext ce("determining free port on "+SSHHostName);

  boost::process::ipstream out;

  SSHCommand sc(SSHHostName, {"bash", "-lc", "isPVFindPort.sh"});
  int ret = boost::process::system(
        sc.command(), boost::process::args(sc.arguments()),
        boost::process::std_out > out,
        boost::process::std_in < boost::process::null
        );

  if (ret!=0)
  {
    throw insight::Exception(
          str( format("Failed to query host %s for free network port!") % SSHHostName)
          );
  }

  std::string outline;
  getline(out, outline);
  std::vector<std::string> parts;
  boost::split(parts, outline, is_any_of(" "));
  if (parts.size()==2)
  {
    if (parts[0]=="PORT")
      return toNumber<int>(parts[1]);
  }

  throw insight::Exception("unexpected answer: \""+outline+"\"");
}


}
