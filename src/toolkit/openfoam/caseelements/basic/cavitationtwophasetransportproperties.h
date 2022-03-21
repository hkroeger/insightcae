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
    declareFactoryTable ( phaseChangeModel, LIST(const ParameterSet& p), LIST(p) );
    declareStaticFunctionTable ( defaultParameters, ParameterSet );
    declareType ( "phaseChangeModel" );

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

};




}




class cavitationTwoPhaseTransportProperties
    : public twoPhaseTransportProperties
{

public:
static void modifyDefaults(ParameterSet& ps);
#include "cavitationtwophasetransportproperties__cavitationTwoPhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> cavitationTwoPhaseTransportProperties Parameters
inherits twoPhaseTransportProperties::Parameters
addTo_makeDefault { modifyDefaults(p); }

psat = double 2300.0 "Saturation pressure"

model = selectablesubset {{ dummy set { } }} dummy "Cavitation model"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

public:
    declareType ( "cavitationTwoPhaseTransportProperties" );
    cavitationTwoPhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
};



} // namespace insight

#endif // INSIGHT_CAVITATIONTWOPHASETRANSPORTPROPERTIES_H
