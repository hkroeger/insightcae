#ifndef INSIGHT_SINGLEPHASETRANSPORTMODEL_H
#define INSIGHT_SINGLEPHASETRANSPORTMODEL_H

#include "openfoam/caseelements/basic/transportmodel.h"

#include "singlephasetransportmodel__singlePhaseTransportProperties__Parameters_headers.h"


namespace insight {




class singlePhaseTransportProperties
    : public transportModel
{

public:
#include "singlephasetransportmodel__singlePhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> singlePhaseTransportProperties Parameters
inherits transportModel::Parameters

nu = double 1e-6 "Kinematic viscosity"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "singlePhaseTransportProperties" );
    singlePhaseTransportProperties ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};




class boussinesqSinglePhaseTransportProperties
    : public singlePhaseTransportProperties
{

public:
#include "singlephasetransportmodel__boussinesqSinglePhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> boussinesqSinglePhaseTransportProperties Parameters
inherits singlePhaseTransportProperties::Parameters

beta = double 3e-3 "beta"
TRef = double 300 "[K] Reference temperature"

Pr = double 0.7 "Laminar prandtl number"
Prt = double 0.85 "Turbulent prandtl number"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "boussinesqSinglePhaseTransportProperties" );
    boussinesqSinglePhaseTransportProperties ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    double density(double T) const;

    static std::string category() { return "Material Properties"; }
};



} // namespace insight

#endif // INSIGHT_SINGLEPHASETRANSPORTMODEL_H
