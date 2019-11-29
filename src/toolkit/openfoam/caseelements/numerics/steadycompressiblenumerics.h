#ifndef INSIGHT_STEADYCOMPRESSIBLENUMERICS_H
#define INSIGHT_STEADYCOMPRESSIBLENUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

namespace insight {

class steadyCompressibleNumerics
    : public FVNumerics
{

public:
#include "steadycompressiblenumerics__steadyCompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> steadyCompressibleNumerics Parameters
inherits FVNumerics::Parameters

nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"

pinternal = double 1e5 "Internal pressure field value"
Tinternal = double 300 "Internal temperature field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

rhoMin = double 0.01 "Lower clipping for density"
rhoMax = double 100. "Upper clipping for density"
transonic = bool true "Check for transsonic flow"
consistent = bool true "Check for SIMPLEC instead of plain SIMPLE"

setup = selection ( accurate stable ) accurate "Select accuratefor second order schemes. In case of stability problems revert to stable."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "steadyCompressibleNumerics" );
    steadyCompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_STEADYCOMPRESSIBLENUMERICS_H
