#include "compressibletwophasethermophysicalproperties.h"


#include "openfoam/ofdicts.h"


namespace insight {




defineType(compressibleTwoPhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(compressibleTwoPhaseThermophysicalProperties);




compressibleTwoPhaseThermophysicalProperties::compressibleTwoPhaseThermophysicalProperties
( OpenFOAMCase& c, const ParameterSet& ps )
  : transportModel(c, ps),
    p_(ps)
{
}




void compressibleTwoPhaseThermophysicalProperties::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    OFDictData::dict& thermophysicalProperties=
        dictionaries.lookupDict("constant/thermophysicalProperties");

    OFDictData::list phasesList;
    for (const auto& p: p_.phases)
    {
        phasesList.push_back(p.name);
    }
    thermophysicalProperties["phases"]=phasesList;
    thermophysicalProperties["pMin"]=p_.pMin;
    thermophysicalProperties["sigma"]=p_.sigma;
}




} // namespace insight
