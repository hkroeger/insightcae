#ifndef INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H
#define INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"

namespace insight {

class buoyantPimpleFoamNumerics
    : public FVNumerics
{

public:
#include "buoyantpimplefoamnumerics__buoyantPimpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> buoyantPimpleFoamNumerics Parameters
inherits FVNumerics::Parameters

time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "buoyantPimpleFoamNumerics" );
    buoyantPimpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};

} // namespace insight

#endif // INSIGHT_BUOYANTPIMPLEFOAMNUMERICS_H
