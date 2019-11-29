#ifndef INSIGHT_SIMPLEDYMFOAMNUMERICS_H
#define INSIGHT_SIMPLEDYMFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/steadyincompressiblenumerics.h"

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

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "simpleDyMFoamNumerics" );
    simpleDyMFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};

} // namespace insight

#endif // INSIGHT_SIMPLEDYMFOAMNUMERICS_H
