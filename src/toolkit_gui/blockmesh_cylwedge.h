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

#ifndef BLOCKMESHDICT_CYLWEDGE_H
#define BLOCKMESHDICT_CYLWEDGE_H

#include "parametersetvisualizer.h"
#include "openfoam/blockmesh_templates.h"
#include "cadtypes.h"

#include "Geom_BoundedCurve.hxx"
#include "gp_Pnt.hxx"
#include "gp_Vec.hxx"

namespace insight
{

namespace bmd
{


class blockMeshDict_CylWedge_ParameterSet_Visualizer;

class blockMeshDict_CylWedge
    : public BlockMeshTemplate
{
  friend class blockMeshDict_CylWedge_ParameterSet_Visualizer;

public:
#include "blockmesh_cylwedge__blockMeshDict_CylWedge__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_CylWedge Parameters

geometry = set
{
    d = double 0.0 "[m] Inner diameter"
    D = double 1.0 "[m] Outer diameter" *necessary
    L = double 1.0 "[m] Length" *necessary
    p0 = vector (0 0 0) "[m] Center point of base surface"
    ex = vector (0 0 1) "[m] Axial direction"
    er = vector (1 0 0) "[m] Radial direction"
    wedge_angle = double 90 "[deg] Wedge angle, symmetric around er" *necessary

    wedge_spine_curve = path ""
"CAD file containing a single curve, which controls the circumferential sweeping of the wedge segment.
The underlying curve needs to be defined from d to D and has to be in the same coordinate system as the target mesh. Errors will occur, if this is not the case.
If the parameter is left blank, a straight radial segment is generated."
}

mesh = set
{
    resolution = selectablesubset {{

     cubical set {
        n_max = int 10 "Number of cells along longest direction. The other directions are discretized with the same cell size but with adjusted number of cells."
     }

     cubical_size set {
        delta = double 0.1 "Uniform cell length."
     }

     individual set {
        nx = int 50 "# cells in axial direction"
        nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
        nu = int 10 "# cells in circumferential direction"
     }

    }} cubical "Mesh resolution"

    gradr = double 1 "grading towards outer boundary"
    core_fraction = double 0.33 "radial extent of core block given as fraction of radius"

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    outerPatchName = string "" "name of patch on outer circumferential surface"
    innerPatchName = string "" "name of patch on inner circumferential surface"
    basePatchName = string "" "name of patch on base end"
    topPatchName = string "" "name of patch on top end"
    cyclmPatchName = string "" "name of patch on cyclic boundary at -0.5*wedge_angle"
    cyclpPatchName = string "" "name of patch on cyclic boundary at +0.5*wedge_angle"

    outerPatchSections = array [ set {
     x0 = double 0 "Beginning of the section. Measured along ex from p0." *necessary
     x1 = double 1 "Beginning of the section. Measured along ex from p0." *necessary
     name = string "" "name of the patch between x0 and x1. If left blank, no different patch will be added
and the section will remain part of outerPatch."
    } ]*0 "Optional axial parts in the outer patch. The outer patch will be split at the specified axial
 coordinates and the surface in between will be given the specified name."
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    arma::mat p0_, ex_, er_, ey_;
    Handle_Geom_Curve spine_;

    std::pair<double,double> limit_angles();
    Handle_Geom_Curve spine();
    arma::mat point_on_spine(double r);

public:
    declareType ( "blockMeshDict_CylWedge" );

    blockMeshDict_CylWedge ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    double rCore() const;
};




class blockMeshDict_CylWedge_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    typedef blockMeshDict_CylWedge::Parameters Parameters;

public:
    void recreateVisualizationElements(UsageTracker* ut, const std::string& blockMeshName );
    void recreateVisualizationElements(UsageTracker* ut) override;
};




}
}

#endif // BLOCKMESHDICT_CYLWEDGE_H
