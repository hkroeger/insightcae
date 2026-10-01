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


#include "rsyncoutputanalyzer.h"
#include "base/stringconv.h"
#include "base/exception.h"
#include "base/externalprocess.h"

#include <deque>
#include <iostream>
#include <mutex>

#include <boost/format.hpp>

namespace insight
{

RSyncOutputAnalyzer::RSyncOutputAnalyzer(std::function<void(int,const std::string&)> progressFunction)
    : progressFunction_(progressFunction),
      pattern(".* ([^ ]*)% *([^ ]*) *([^ ]*)")
{}


void RSyncOutputAnalyzer::update(const std::string& line)
{
    boost::smatch match;
    if (boost::regex_search( line, match, pattern, boost::match_default ))
    {
      int percent=toNumber<int>(match[1]);
      std::string rate=match[2];
      std::string elapsed=match[3];
      //      int i_file=toNumber<int>(match[4]);
      //      std::string ir_or_to(match[5]);
      //      int i_to_chk=toNumber<int>(match[6]);
      //      int total_to_chk=toNumber<int>(match[7]);

      if (progressFunction_)
      {
                progressFunction_(
                    percent,
                    str(boost::format("%s, %s") % rate % elapsed)
                    );
      }
    }
}




void runRSync(
    Job& job,
    std::function<void(int,const std::string&)> progressFunction )
{
    RSyncOutputAnalyzer rpa(progressFunction);

    // keep the last error lines for the error message
    std::mutex mx;
    std::deque<std::string> errLines;

    job.ios_run_with_interruption(

        [&](const std::string& line)
        {
            std::cout<<line<<std::endl; // mirror to console
            rpa.update(line);
        },

        [&](const std::string& line)
        {
            std::cout<<"[E] "<<line<<std::endl; // mirror to console
            if (!line.empty())
            {
                std::lock_guard<std::mutex> l(mx);
                errLines.push_back(line);
                if (errLines.size()>10) errLines.pop_front();
            }
        }
    );

    job.wait();

    int ret = job.process().exit_code();
    if (ret!=0)
    {
        std::string msg;
        for (const auto& l: errLines)
            msg += "\n  "+l;
        throw insight::Exception(
            "file transfer with rsync failed (exit code %d)%s",
            ret, msg.c_str() );
    }
}


} // namespace insight
