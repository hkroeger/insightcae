#ifndef INSIGHT_SINGLEPHASETRANSPORTMODEL_H
#define INSIGHT_SINGLEPHASETRANSPORTMODEL_H

#include "openfoam/caseelements/basic/transportmodel.h"

namespace insight {

class singlePhaseTransportProperties
    : public transportModel
{

public:
#include "singlephasetransportmodel__singlePhaseTransportProperties__Parameters.h"
/*
PARAMETERSET>>> singlePhaseTransportProperties Parameters

nu = double 1e-6 "Kinematic viscosity"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "singlePhaseTransportProperties" );
    singlePhaseTransportProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};

} // namespace insight

#endif // INSIGHT_SINGLEPHASETRANSPORTMODEL_H
