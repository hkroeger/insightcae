#ifndef INSIGHT_CAVITATINGFOAMNUMERICS_H
#define INSIGHT_CAVITATINGFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "cavitatingfoamnumerics__cavitatingFoamNumerics__Parameters_headers.h"

namespace insight {

class cavitatingFoamNumerics
    : public FVNumerics
{

public:
#include "cavitatingfoamnumerics__cavitatingFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> cavitatingFoamNumerics Parameters
inherits FVNumerics::Parameters

solverName = string "cavitatingFoam" "Name of the solver"
pamb = double 1e5 "Ambient pressure value"
rhoamb = double 1 "Ambient density"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "cavitatingFoamNumerics" );
    cavitatingFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_CAVITATINGFOAMNUMERICS_H
