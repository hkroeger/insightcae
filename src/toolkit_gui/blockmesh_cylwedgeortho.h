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


#ifndef BLOCKMESH_CYLWEDGEORTHO_H
#define BLOCKMESH_CYLWEDGEORTHO_H

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



class blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer;





class blockMeshDict_CylWedgeOrtho
    : public BlockMeshTemplate
{
  friend class blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer;

public:
#include "blockmesh_cylwedgeortho__blockMeshDict_CylWedgeOrtho__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_CylWedgeOrtho Parameters

geometry = set
{
    L = double 1.0 "[m] Length" *necessary
    p0 = vector (0 0 0) "[m] Center point of axis. Mesh extrusion will start on axial location of spine base point, regardless of this parameter."
    ex = vector (0 0 1) "[m] Axial direction"
    wedge_angle = double 90 "[deg] Wedge angle, symmetric around er" *necessary

    wedge_spine_curve = path ""
"CAD file containing a single curve, which controls the circumferential sweeping of the wedge segment.
The underlying curve needs to be defined from d to D and has to be in the same coordinate system as the target mesh. Errors will occur, if this is not the case.
If the parameter is left blank, a straight radial segment is generated."

    inner_interface = selectablesubset {{

     none set {}

     extend set {
      distance = double 1 "Protrusion distance (towards inside)" *necessary
      z0 = double 0 "lower z-coordinate, measured from p0" *necessary
      z1 = double 1 "upper z-coordinate, measured from p0" *necessary
     }

    }} none "Protrusion for interface at inner cylindrical boundary"

    outer_interface = selectablesubset {{

     none set {}

     extend set {
      distance = double 1 "Protrusion distance (towards inside)" *necessary
      z0 = double 0 "lower z-coordinate, measured from p0" *necessary
      z1 = double 1 "upper z-coordinate, measured from p0" *necessary
     }

    }} none "Protrusion for interface at outer cylindrical boundary"

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
    outerInterfacePatchName = string "" "name of protrusion patch on outer circumferential surface"
    innerInterfacePatchName = string "" "name of protrusion patch on inner circumferential surface"
    basePatchName = string "" "name of patch on base end"
    topPatchName = string "" "name of patch on top end"
    cyclmPatchName = string "" "name of patch on cyclic boundary at -0.5*wedge_angle"
    cyclpPatchName = string "" "name of patch on cyclic boundary at +0.5*wedge_angle"
}

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    struct Patches
    {
      Patch* base=nullptr;
      Patch* top=nullptr;
      Patch* outer=nullptr;
      Patch* inner=nullptr;
      Patch* outerif=nullptr;
      Patch* innerif=nullptr;
      Patch* pcyclm=nullptr;
      Patch* pcyclp=nullptr;
    };

    void insertBlocks
    (
        Handle_Geom_Curve spine_rvs,  // first param: inside@r0, last param: outside@r1
        double t0, double t1,
        double angle,
        gp_Vec ez, gp_Pnt center,
        double z0, double z1,
        Patches& pc,
        const Parameters::geometry_type::inner_interface_type& pro_inner,
        const Parameters::geometry_type::outer_interface_type& pro_outer,
        bool no_top_edg,
        bool is_lowest,
        int nuBy2, int nx, int nr, double deltax
    );

public:
    declareType ( "blockMeshDict_CylWedgeOrtho" );

    blockMeshDict_CylWedgeOrtho ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    double rCore() const;
};



class blockMeshDict_CylWedgeOrtho_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    typedef blockMeshDict_CylWedgeOrtho::Parameters Parameters;

public:
    void recreateVisualizationElements(UsageTracker* ut, const std::string& blockMeshName );
    void recreateVisualizationElements(UsageTracker* ut ) override;
};

}

}

#endif // BLOCKMESH_CYLWEDGEORTHO_H
