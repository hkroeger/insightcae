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

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

    double rCore() const;
};


}
}

#endif // BLOCKMESHDICT_CURVEDCYLINDER_H
