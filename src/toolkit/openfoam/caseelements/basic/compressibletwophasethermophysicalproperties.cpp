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

    for (const auto& p: p_.phases)
    {
        SpeciesData sd(p);

        OFDictData::dict& td=
            dictionaries.lookupDict("constant/thermophysicalProperties."+p.name);

        OFDictData::dict tt;
        tt["type"]="heRhoThermo";
        tt["mixture"]="pureMixture";
        tt["transport"]=sd.transportType();
        tt["thermo"]=sd.thermoType();
        switch (p_.energyType)
        {
        case Parameters::energyType_type::sensibleInternalEnthalpy:
            tt["energy"]="sensibleInternalEnthalpy";
            break;
        case Parameters::energyType_type::sensibleInternalEnergy:
            tt["energy"]="sensibleInternalEnergy";
            break;
        default:
            throw insight::UnhandledSelection();
        }
        tt["equationOfState"]=sd.equationOfStateType();
        tt["specie"]="specie";
        td["thermoType"]=tt;

        OFDictData::dict tsd;
        sd.insertSpecieEntries(tsd);
        sd.insertThermodynamicsEntries(tsd);
        sd.insertEquationOfStateEntries(tsd);
        sd.insertTransportEntries(tsd);
        sd.insertElementsEntries(tsd);
        td["mixture"]=tsd;
    }
}




} // namespace insight
