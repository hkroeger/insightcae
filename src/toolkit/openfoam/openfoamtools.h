/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef OPENFOAMTOOLS_H
#define OPENFOAMTOOLS_H

#include <string>
#include <vector>

#include "boost/filesystem.hpp"

#include "openfoam/openfoamcase.h"

namespace insight
{
  
typedef std::map<double, boost::filesystem::path> TimeDirectoryList;

TimeDirectoryList listTimeDirectories(const boost::filesystem::path& dir);
  
void setSet(const OpenFOAMCase& ofc, const boost::filesystem::path& location, const std::vector<std::string>& cmds);

void setsToZones(const OpenFOAMCase& ofc, const boost::filesystem::path& location, bool noFlipMap=true);

}

#endif // OPENFOAMTOOLS_H
