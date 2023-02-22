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
    declareType ( "phaseChangeModel" );
    declareDynamicClass(phaseChangeModel);

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

n = double 1.6e13 "n"
dNuc = double 2.0e-6 "dNuc"
Cc = double 1.0 "Cc"
Cv = double 1.0 "Cv"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "SchnerrSauer" );
    SchnerrSauer ( const ParameterSet& p );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    ParameterSet getParameters() const override { return Parameters::makeDefault(); }
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

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "cavitationTwoPhaseTransportProperties" );
    cavitationTwoPhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_CAVITATIONTWOPHASETRANSPORTPROPERTIES_H
