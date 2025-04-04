#ifndef INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H
#define INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"
#include "buoyantpimplefoamnumerics__buoyantPimpleFoamNumerics__Parameters_headers.h"

namespace insight {

class buoyantPimpleFoamNumerics
    : public FVNumerics
{

public:
#include "buoyantpimplefoamnumerics__buoyantPimpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> buoyantPimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

boussinesqApproach = bool false "Use boussinesq approach"
time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"

Tinternal = double 300 "initial temperature in internal field"
pinternal = double 1e5 "initial pressure in internal field. Should be zero, if boussinesqApproach is set to true and some physical reasonable absolut pressure value otherwise."

createGetter
<<<PARAMETERSET
*/

protected:
    void init();

public:
    declareType ( "buoyantPimpleFoamNumerics" );
    buoyantPimpleFoamNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H
