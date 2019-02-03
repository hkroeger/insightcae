#ifndef BLOCKMESH_TUBEMESH_H
#define BLOCKMESH_TUBEMESH_H

#include "blockmesh_curvedcylinder.h"

namespace insight {

namespace bmd
{



/**
 * A cylindrical tube meshed with an O-grid
 */

class blockMeshDict_TubeMesh
: public BlockMeshTemplate
{
public:
#include "blockmesh_tubemesh__blockMeshDict_TubeMesh__Parameters.h"
/*
PARAMETERSET>>> blockMeshDict_TubeMesh Parameters

geometry = set
{
    D = double 1.0 "[m] Diameter"
    wire = path "geometry.brep" "Geometry file with a single wire inside"
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
    declareType ( "blockMeshDict_TubeMesh" );

    blockMeshDict_TubeMesh ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
};


}
}

#endif // BLOCKMESH_TUBEMESH_H
