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


#ifndef INSIGHT_BLOCKMESH_TEMPLATES_H
#define INSIGHT_BLOCKMESH_TEMPLATES_H

#include "openfoam/blockmesh.h"
#include "blockmesh_templates__blockMeshDict_Cylinder__Parameters_headers.h"

namespace insight {
  
namespace bmd
{
  
    
    
    
class BlockMeshTemplate
    : public insight::bmd::blockMesh
{
public:
    BlockMeshTemplate(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    virtual void create_bmd () =0;

    static arma::mat correct_trihedron(arma::mat& ex, arma::mat &ez);
};

  



/**
 * A cylinder meshed with an O-grid
 */

class blockMeshDict_Cylinder
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Cylinder__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Cylinder Parameters
inherits BlockMeshTemplate::Parameters

geometry = set
{
    D = double 1.0 "[m] Outer diameter" *necessary
    d = double 0.0 "[m] Inner diameter" *necessary
    L = double 1.0 "[m] Length" *necessary
    p0 = vector (0 0 0) "[m] Center point of base surface"
    ex = vector (0 0 1) "[m] Axial direction"
    er = vector (1 0 0) "[m] Radial direction"
}

mesh = set
{
    resolution = selectablesubset {{

     cubical set {
        n_max = int 10 "Number of cells along longest direction. The other directions are discretized with the same cell size but with adjusted number of cells."
        xcubical = double 0.5 "Cubical cells can only be assured at a single radius. This radius is specified dimensionless as $x=\\frac{r_{cube}-r_i}{r_a-r_i}$."
     }

     cubical_size set {
        delta = double 0.1 "Uniform cell length."
        xcubical = double 0.5 "Cubical cells can only be assured at a single radius. This radius is specified dimensionless as $x=\\frac{r_{cube}-r_i}{r_a-r_i}$."
     }

     individual set {
        nx = int 50 "# cells in axial direction"
        nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
        nu = int 10 "# cells in circumferential direction"
     }

    }} cubical "Mesh resolution"

    gradr = double 1 "grading towards outer boundary"
    gradax = double 1 "grading towards top boundary"

    topology = selectablesubset {{

     oGrid set {
        core_fraction = double 0.33 "core block radius given as fraction of outer diameter"
        smoothCore = bool false "use splines as boundaries of core block and attempt to create a uniform distance from core border to outer perimeter"
     }

     pieSlice set {
     }

    }} oGrid "Topology of the grid. Note: if a nonzero inner radius is nonzero, pieSlice is always used."

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    circumPatchName = string "" "name of patch on outer circumferential surface"
    innerPatchName = string "" "name of patch on inner circumferential surface"
    basePatchName = string "" "name of patch on base end"
    topPatchName = string "" "name of patch on top end"

    cellZoneName = string "" "name of the zone into which all cells are to be included"
}

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "blockMeshDict_Cylinder" );

    blockMeshDict_Cylinder ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

    virtual void create_bmd();
};








/**
 * A simple rectangular mesh
 */

class blockMeshDict_Box
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Box__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Box Parameters
inherits BlockMeshTemplate::Parameters

geometry = set
{
    L = double 1.0 "[m] Length (X)" *necessary
    W = double 1.0 "[m] Width (Y)" *necessary
    H = double 1.0 "[m] Height (Z)" *necessary
    p0 = vector(0 0 0) "[m] Lower left corner" *necessary
    ex = vector(1 0 0) "[m] X direction"
    ez = vector(0 0 1) "[m] Upward (Z) direction"
}

mesh = set
{
    resolution = selectablesubset {{

     cubical set {
        n_max = int 10 "Number of cells along longest side. The other sides are discretized with the same cell size but with adjusted number of cells."
     }

     cubical_size set {
        delta = double 0.1 "Uniform cell length."
     }

     individual set {
        nx = int 10 "# cells in X direction"
        ny = int 10 "# cells in Y direction"
        nz = int 10 "# cells in Z direction"
     }

    }} cubical "Mesh resolution"

    defaultPatchName = string "walls" "name of patch where all patches with empty names are assigned to."
    XpPatchName = string "" "name of patch on forward (+X) side"
    XmPatchName = string "" "name of patch on rearward (-X) side"
    YpPatchName = string "" "name of patch on right (+Y) side"
    YmPatchName = string "" "name of patch on left (-Y) side"
    ZpPatchName = string "" "name of patch on top (+Z) side"
    ZmPatchName = string "" "name of patch on bottom (-Z) side"
}

createGetters
<<<PARAMETERSET
*/

public:
    declareType ( "blockMeshDict_Box" );

    blockMeshDict_Box ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

    virtual void create_bmd();

};






/**
 * A simple spherical mesh
 */

class blockMeshDict_Sphere
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_Sphere__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_Sphere Parameters
inherits BlockMeshTemplate::Parameters

geometry = set
{
    D = double 1.0 "[m] Sphere diameter" *necessary
    center = vector(0 0 0) "[m] Sphere center"

    core_fraction = double 0.2 "Edge length of the core block"

    ex = vector (1 0 0) "X-Direction (first edge direction of core block)"
    ez = vector (0 0 1) "Z-Direction (second edge direction of core block)"
}

mesh = set
{
    grad_r = double 1 "Grading towards outer boundary (ratio of outer cell length to inner cell length)"
    n_u = int 10 "Number of cells in circumferential direction"

    theta_trans = double 50 "latitude of the block border"

    outerPatchName = string "outer" "name of boundary patch."
}

createGetters
<<<PARAMETERSET
*/

protected:
    Patch* outer_;

public:
    declareType ( "blockMeshDict_Sphere" );

    blockMeshDict_Sphere ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );

    void create_bmd() override;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

};


}

}

#endif
