#ifndef BLOCKMESHDICT_CYLWEDGE_H
#define BLOCKMESHDICT_CYLWEDGE_H

#include "openfoam/blockmesh_templates.h"

namespace insight {

namespace bmd
{

/**
 * A cylinder meshed with an O-grid
 */

class blockMeshDict_CylWedge
    : public BlockMeshTemplate
{
public:
#include "blockmesh_templates__blockMeshDict_CylWedge__Parameters.h"
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
    nx = int 50 "# cells in axial direction"
    nr = int 10 "# cells in radial direction (from edge of core block to outer radius)"
    nu = int 10 "# cells in circumferential direction"
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
    declareType ( "blockMeshDict_CylWedge" );

    blockMeshDict_CylWedge ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    virtual void create_bmd();

    inline static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }

    double rCore() const;
};

}
}

#endif // BLOCKMESHDICT_CYLWEDGE_H
