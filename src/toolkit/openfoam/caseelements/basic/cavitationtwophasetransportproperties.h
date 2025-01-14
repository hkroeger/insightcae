#ifndef INSIGHT_CAVITATIONTWOPHASETRANSPORTPROPERTIES_H
#define INSIGHT_CAVITATIONTWOPHASETRANSPORTPROPERTIES_H

#include "openfoam/caseelements/basic/twophasetransportproperties.h"

#include "cavitationtwophasetransportproperties__SchnerrSauer__Parameters_headers.h"

namespace insight {

namespace phaseChangeModels
{




class phaseChangeModel
{
public:
#include "cavitationtwophasetransportproperties__phaseChangeModel__Parameters.h"
/*
PARAMETERSET>>> phaseChangeModel Parameters

createGetters
<<<PARAMETERSET
*/

public:
    declareType ( "phaseChangeModel" );
    declareDynamicClass(phaseChangeModel);

    phaseChangeModel(ParameterSetInput ip = ParameterSetInput() );
    virtual ~phaseChangeModel();
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const =0;
};



class SchnerrSauer
    : public phaseChangeModel
{

public:
#include "cavitationtwophasetransportproperties__SchnerrSauer__Parameters.h"
/*
PARAMETERSET>>> SchnerrSauer Parameters
inherits phaseChangeModel::Parameters

n = double 1.6e13 "n"
dNuc = double 2.0e-6 "dNuc"
Cc = double 1.0 "Cc"
Cv = double 1.0 "Cv"

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "SchnerrSauer" );
    SchnerrSauer ( ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};




}




class cavitationTwoPhaseTransportProperties
    : public twoPhaseTransportProperties
{

public:
#include "cavitationtwophasetransportproperties__cavitationTwoPhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> cavitationTwoPhaseTransportProperties Parameters
inherits twoPhaseTransportProperties::Parameters

psat = double 2300.0 "Saturation pressure"

model = dynamicclassconfig "insight::phaseChangeModels::phaseChangeModel" default "SchnerrSauer" "Cavitation model"

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "cavitationTwoPhaseTransportProperties" );
    cavitationTwoPhaseTransportProperties ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_CAVITATIONTWOPHASETRANSPORTPROPERTIES_H
