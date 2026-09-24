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

#ifndef INSIGHT_OFTIMEDIRECTORIES_H
#define INSIGHT_OFTIMEDIRECTORIES_H

#include <map>
#include <string>

#include <boost/filesystem.hpp>

#ifdef SWIG
%template(TimeDirectoryList) std::map<double, boost::filesystem::path>;
#endif

namespace insight
{

typedef std::map<double, boost::filesystem::path> TimeDirectoryList;

TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir, const boost::filesystem::path& fileInsideToAppend = "");

// std::string for compat with SWIG wrapper
std::string getLatestTimeDirectory(const boost::filesystem::path& dir);

}

#endif // INSIGHT_OFTIMEDIRECTORIES_H
