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


#ifndef INSIGHT_RSYNCOUTPUTANALYZER_H
#define INSIGHT_RSYNCOUTPUTANALYZER_H

#include <functional>
#include <string>

#include <boost/regex.hpp>

#include "base/outputanalyzer.h"

namespace insight {

class Job;

class RSyncOutputAnalyzer
    : public OutputAnalyzer
{
  std::function<void(int,const std::string&)> progressFunction_;
  boost::regex pattern;

public:
  RSyncOutputAnalyzer(std::function<void(int,const std::string&)> progressFunction);
  void update(const std::string& line) override;
};


/**
 * @brief runRSync
 * run an rsync job (started with "--info=progress2"),
 * report its progress and wait for its end.
 * Throws, if rsync fails. The message contains the error output of rsync.
 */
void runRSync(
    Job& job,
    std::function<void(int,const std::string&)> progressFunction );

}

#endif // INSIGHT_RSYNCOUTPUTANALYZER_H
