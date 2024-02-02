#ifndef INSIGHT_COMPRESSIBLETWOPHASETHERMOPHYSICALPROPERTIES_H
#define INSIGHT_COMPRESSIBLETWOPHASETHERMOPHYSICALPROPERTIES_H

#include "openfoam/caseelements/basic/transportmodel.h"
#include "openfoam/caseelements/thermophysicalcaseelements.h"

namespace insight {

class compressibleTwoPhaseThermophysicalProperties
    : public transportModel
{

public:
#include "compressibletwophasethermophysicalproperties__compressibleTwoPhaseThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleTwoPhaseThermophysicalProperties Parameters

phases = array [
     includedset "insight::SpeciesData::Parameters"
    ] *2 "Definitions of the phases"

energyType = selection (sensibleInternalEnthalpy sensibleInternalEnergy) sensibleInternalEnthalpy ""

sigma = double 0.07 "Surface tension"

pMin = double 0 "[Pa] minimum pressure"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "compressibleTwoPhaseThermophysicalProperties" );
    compressibleTwoPhaseThermophysicalProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};

} // namespace insight

#endif // INSIGHT_COMPRESSIBLETWOPHASETHERMOPHYSICALPROPERTIES_H
