#ifndef INSIGHT_STEADYINCOMPRESSIBLENUMERICS_H
#define INSIGHT_STEADYINCOMPRESSIBLENUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

namespace insight {

class steadyIncompressibleNumerics
    : public FVNumerics
{

public:
#include "steadyincompressiblenumerics__steadyIncompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> steadyIncompressibleNumerics Parameters
inherits FVNumerics::Parameters

checkResiduals = bool true "Whether to check residuals during run"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "steadyIncompressibleNumerics" );
    steadyIncompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault(), const std::string& pName="p" );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};

} // namespace insight

#endif // INSIGHT_STEADYINCOMPRESSIBLENUMERICS_H
