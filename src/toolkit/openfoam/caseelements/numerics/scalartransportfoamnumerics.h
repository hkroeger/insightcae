#ifndef INSIGHT_SCALARTRANSPORTFOAMNUMERICS_H
#define INSIGHT_SCALARTRANSPORTFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

#include "scalartransportfoamnumerics__scalarTransportFoamNumerics__Parameters_headers.h"

namespace insight {

class scalarTransportFoamNumerics
        : public insight::FVNumerics
{

public:
#include "scalartransportfoamnumerics__scalarTransportFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> scalarTransportFoamNumerics Parameters
inherits FVNumerics::Parameters

Tinternal = double 300.0 "[K] Initial temperature value in internal field"
Uinternal = vector (0 0 0) "[m/s] Initial velocity value in internal field"
DT = double 1e-6 "[m^2/s] Constant diffivity"

nNonOrthogonalCorrectors = int 0 "Number of correctors for non-orthogonality errors"

createGetters
<<<PARAMETERSET
*/

public:
    declareType ( "scalarTransportFoamNumerics" );
    scalarTransportFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_SCALARTRANSPORTFOAMNUMERICS_H
