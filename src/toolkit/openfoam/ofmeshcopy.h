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

#ifndef INSIGHT_OFMESHCOPY_H
#define INSIGHT_OFMESHCOPY_H

#include <boost/filesystem.hpp>

namespace insight
{

class OFEnvironment;

/*
 * Copy polyMesh directory below "from" into "to"
 * "to" is created, if nonexistent
 * Copy only basic mesh description, if "purify" is set
 */
void copyPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to,
		  bool purify=false, bool ignoremissing=false, bool include_zones=false);

/**
 * create mesh as symbolic links from other case.
 * If env is given, create a case skeleton in target case
 */
void linkPolyMesh(const boost::filesystem::path& from, const boost::filesystem::path& to,
                  const OFEnvironment* env=NULL);

/*
 * Copy field files below "from" into "to"
 * "to" is created, if nonexistent
 */
void copyFields(const boost::filesystem::path& from, const boost::filesystem::path& to);

}

#endif // INSIGHT_OFMESHCOPY_H
