#ifndef INSIGHT_POTENTIALFOAMNUMERICS_H
#define INSIGHT_POTENTIALFOAMNUMERICS_H


#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "potentialfoamnumerics__potentialFoamNumerics__Parameters_headers.h"

namespace insight {

class potentialFoamNumerics
    : public FVNumerics
{

public:
#include "potentialfoamnumerics__potentialFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> potentialFoamNumerics Parameters
inherits FVNumerics::Parameters

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "potentialFoamNumerics" );
    potentialFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_POTENTIALFOAMNUMERICS_H
