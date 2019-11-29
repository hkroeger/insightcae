#ifndef INSIGHT_MAGNETICFOAMNUMERICS_H
#define INSIGHT_MAGNETICFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

namespace insight {

class magneticFoamNumerics
    : public FVNumerics
{

public:
#include "magneticfoamnumerics__magneticFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> magneticFoamNumerics Parameters
inherits FVNumerics::Parameters

solverName = string "magneticFoam" "Name of the solver to use"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "magneticFoamNumerics" );
    magneticFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};


} // namespace insight

#endif // INSIGHT_MAGNETICFOAMNUMERICS_H
