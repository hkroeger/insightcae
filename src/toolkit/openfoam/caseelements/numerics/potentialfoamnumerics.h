#ifndef INSIGHT_POTENTIALFOAMNUMERICS_H
#define INSIGHT_POTENTIALFOAMNUMERICS_H


#include "openfoam/caseelements/numerics/fvnumerics.h"

namespace insight {

class potentialFoamNumerics
    : public FVNumerics
{

public:
#include "potentialfoamnumerics__potentialFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> potentialFoamNumerics Parameters
inherits FVNumerics::Parameters

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "potentialFoamNumerics" );
    potentialFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_POTENTIALFOAMNUMERICS_H
