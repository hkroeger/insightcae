#ifndef INSIGHT_MAGNETICFOAMNUMERICS_H
#define INSIGHT_MAGNETICFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "magneticfoamnumerics__magneticFoamNumerics__Parameters_headers.h"

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

createGetters
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "magneticFoamNumerics" );
    magneticFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};


} // namespace insight

#endif // INSIGHT_MAGNETICFOAMNUMERICS_H
