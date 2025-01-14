#ifndef INSIGHT_SIMPLEDYMFOAMNUMERICS_H
#define INSIGHT_SIMPLEDYMFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"

#include "simpledymfoamnumerics__simpleDyMFoamNumerics__Parameters_headers.h"

namespace insight {

class simpleDyMFoamNumerics
    : public steadyIncompressibleNumerics
{

public:
#include "simpledymfoamnumerics__simpleDyMFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> simpleDyMFoamNumerics Parameters
inherits steadyIncompressibleNumerics::Parameters

FEMinterval = int 10 "Interval between successive FEM updates"

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "simpleDyMFoamNumerics" );
    simpleDyMFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_SIMPLEDYMFOAMNUMERICS_H
