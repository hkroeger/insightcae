#ifndef INSIGHT_REACTINGFOAMNUMERICS_H
#define INSIGHT_REACTINGFOAMNUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"
#include "openfoam/caseelements/numerics/pimplesettings.h"

#include "reactingfoamnumerics__reactingFoamNumerics__Parameters_headers.h"

namespace insight
{

class reactingFoamNumerics
    : public FVNumerics
{

public:
#include "reactingfoamnumerics__reactingFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> reactingFoamNumerics Parameters
inherits FVNumerics::Parameters

time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"

forceLES = bool false "Whether to enforce LES numerics"

buoyancy = bool false "Whether to use a buoyancy formulation"

pinternal = double 1e5 "[Pa] initial pressure in domain"
Tinternal = double 300. "[K] initial temperature in domain"
Uinternal = vector (0 0 0) "[m/s] initial velocity in domain"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

    void init();

public:
    declareType ( "reactingFoamNumerics" );
    reactingFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

    inline const reactingFoamNumerics::Parameters& parameters() const { return p_; }
};

} // namespace insight

#endif // INSIGHT_REACTINGFOAMNUMERICS_H
