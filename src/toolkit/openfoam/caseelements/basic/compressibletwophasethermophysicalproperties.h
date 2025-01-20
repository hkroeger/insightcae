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
inherits transportModel::Parameters

phases = labeledarray "phase%d" [
     includedset "insight::SpeciesData::Parameters"
    ] *2 "Definitions of the phases"

energyType = selection (sensibleInternalEnthalpy sensibleInternalEnergy) sensibleInternalEnthalpy ""

sigma = double 0.07 "Surface tension"

pMin = double 0 "[Pa] minimum pressure"

createGetter
<<<PARAMETERSET
*/


public:
    declareType ( "compressibleTwoPhaseThermophysicalProperties" );
    compressibleTwoPhaseThermophysicalProperties ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};

} // namespace insight

#endif // INSIGHT_COMPRESSIBLETWOPHASETHERMOPHYSICALPROPERTIES_H
