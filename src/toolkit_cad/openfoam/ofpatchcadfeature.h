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


#ifndef INSIGHT_CAD_OFPATCHCADFEATURE_H
#define INSIGHT_CAD_OFPATCHCADFEATURE_H

#include "cadtypes.h"
#include "openfoam/ofpolymeshreader.h"

namespace insight {
namespace cad {


/**
 * @brief readOpenFOAMPatchAsFeature
 * reads the boundary patches matching the given regular expression
 * directly from the polyMesh files (no OpenFOAM installation required)
 * and returns them as a triangulated surface feature (cad::STL)
 */
FeaturePtr readOpenFOAMPatchAsFeature(
    const boost::filesystem::path& caseDir,
    const std::string& patchNamePattern,
    const std::string& regionName = std::string(),
    const std::string& time = "constant" );

FeaturePtr toFeature(const OFPatchSurface& surf);


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_OFPATCHCADFEATURE_H
