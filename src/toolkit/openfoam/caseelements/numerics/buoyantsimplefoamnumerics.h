#ifndef INSIGHT_BUOYANTSIMPLEFOAMNUMERICS_H
#define INSIGHT_BUOYANTSIMPLEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"
#include "buoyantsimplefoamnumerics__buoyantSimpleFoamNumerics__Parameters_headers.h"

namespace insight {

class buoyantSimpleFoamNumerics
    : public FVNumerics
{

public:
#include "buoyantsimplefoamnumerics__buoyantSimpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> buoyantSimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

boussinesqApproach = bool false "Use boussinesq approach"
checkResiduals = bool false "Enable solver stop on residual goal"
nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"

Tinternal = double 300 "initial temperature in internal field"
pinternal = double 1e5 "initial pressure in internal field"

createGetter
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "buoyantSimpleFoamNumerics" );
    buoyantSimpleFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_BUOYANTSIMPLEFOAMNUMERICS_H
