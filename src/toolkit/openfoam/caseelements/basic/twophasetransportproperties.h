#ifndef INSIGHT_TWOPHASETRANSPORTPROPERTIES_H
#define INSIGHT_TWOPHASETRANSPORTPROPERTIES_H

#include "openfoam/caseelements/basic/transportmodel.h"

#include "twophasetransportproperties__twoPhaseTransportProperties__Parameters_headers.h"


namespace insight {

class twoPhaseTransportProperties
    : public transportModel
{

public:
#include "twophasetransportproperties__twoPhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> twoPhaseTransportProperties Parameters

nu1 = double 1e-6 "Kinematic viscosity of fluid 1"
rho1 = double 1025.0 "Density of fluid 1"
nu2 = double 1.5e-5 "Kinematic viscosity of fluid 2"
rho2 = double 1.0 "Density of fluid 2"
sigma = double 0.07 "Surface tension"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "twoPhaseTransportProperties" );
    twoPhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};

} // namespace insight

#endif // INSIGHT_TWOPHASETRANSPORTPROPERTIES_H
