#include "compressibletwophasethermophysicalproperties.h"


#include "openfoam/ofdicts.h"


namespace insight {




defineType(compressibleTwoPhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(compressibleTwoPhaseThermophysicalProperties);




compressibleTwoPhaseThermophysicalProperties::compressibleTwoPhaseThermophysicalProperties
( OpenFOAMCase& c, ParameterSetInput ip )
    : transportModel(c, ip.forward<Parameters>())
{
}




void compressibleTwoPhaseThermophysicalProperties::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    OFDictData::dict& thermophysicalProperties=
        dictionaries.lookupDict("constant/thermophysicalProperties");

    OFDictData::list phasesList;
    for (const auto& ph: p().phases)
    {
        phasesList.push_back(ph.name);
    }
    thermophysicalProperties["phases"]=phasesList;
    thermophysicalProperties["pMin"]=p().pMin;
    thermophysicalProperties["sigma"]=p().sigma;

    for (const auto& ph: p().phases)
    {
        SpeciesData sd(ph);

        OFDictData::dict& td=
            dictionaries.lookupDict("constant/thermophysicalProperties."+ph.name);

        OFDictData::dict tt;
        tt["type"]="heRhoThermo";
        tt["mixture"]="pureMixture";
        tt["transport"]=sd.transportType();
        tt["thermo"]=sd.thermoType();
        switch (Parameters::energyType_type(p().energyType))
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
