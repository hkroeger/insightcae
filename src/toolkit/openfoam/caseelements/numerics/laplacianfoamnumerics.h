#ifndef INSIGHT_LAPLACIANFOAMNUMERICS_H
#define INSIGHT_LAPLACIANFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

namespace insight {

class laplacianFoamNumerics
    : public FVNumerics
{

public:
#include "laplacianfoamnumerics__laplacianFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> laplacianFoamNumerics Parameters
inherits FVNumerics::Parameters

Tinternal = double 300.0 "[K] Initial temperature value in internal field"
DT = double 1e-6 "[m^2/s] Constant diffivity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "laplacianFoamNumerics" );
    laplacianFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_LAPLACIANFOAMNUMERICS_H
