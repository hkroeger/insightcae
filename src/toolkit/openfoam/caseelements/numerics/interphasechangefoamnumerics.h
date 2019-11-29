#ifndef INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H
#define INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/interfoamnumerics.h"

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

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "interPhaseChangeFoamNumerics" );
    interPhaseChangeFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_INTERPHASECHANGEFOAMNUMERICS_H
