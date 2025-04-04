#ifndef INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H
#define INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/interfoamnumerics.h"
#include "interphasechangefoamnumerics__interPhaseChangeFoamNumerics__Parameters_headers.h"

namespace insight {

class interPhaseChangeFoamNumerics
    : public interFoamNumerics
{

public:
#include "interphasechangefoamnumerics__interPhaseChangeFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> interPhaseChangeFoamNumerics Parameters
inherits interFoamNumerics::Parameters

solverName = string "interPhaseChangeFoam" "Name of the solver to use"

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "interPhaseChangeFoamNumerics" );
    interPhaseChangeFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H
