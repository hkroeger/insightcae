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
 */

#ifndef BLOCKMESHDICT_CURVEDCYLINDER_H
#define BLOCKMESHDICT_CURVEDCYLINDER_H

#include "openfoam/blockmesh_templates.h"

namespace insight {

namespace bmd
{



/**
 * A cylindrical tube meshed with an O-grid
 */

class blockMeshDict_CurvedCylinder
: public BlockMeshTemplate
{
public:
#include "blockmesh_curvedcylinder__blockMeshDict_CurvedCylinder__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_CurvedCylinder Parameters

geometry = set
{
    D = double 1.0 "[m] Diameter"
    p0 = vector (0 0 0) "[m] Center point of base surface"
    ex = vector (0 0 1) "[m] Axial direction (tangential to arc spine in p0)"
    er = vector (1 0 0) "[m] Radial direction"
    p1 = vector (1 0 1) "[m] Center point of top surface"
}

mesh = set
{
    nx = int 50 "# cells in axial direction"
    nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
    nu = int 10 "# cells in circumferential direction (in one of four segments)"
    gradr = double 1 "grading towards outer boundary"
    core_fraction = double 0.33 "radial extent of core block given as fraction of radius"

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    circumPatchName = string "" "name of patch on circumferential surface"
    basePatchName = string "" "name of patch on base end"
    topPatchName = string "" "name of patch on top end"
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "blockMeshDict_CurvedCylinder" );

    blockMeshDict_CurvedCylinder ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    CoordinateSystem calc_end_CS() const;

    virtual void create_bmd();

    double rCore() const;
};


}
}

#endif // BLOCKMESHDICT_CURVEDCYLINDER_H
