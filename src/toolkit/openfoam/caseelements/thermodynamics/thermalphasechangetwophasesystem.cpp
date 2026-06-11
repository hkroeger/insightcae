#include "thermalphasechangetwophasesystem.h"
#include "openfoam/caseelements/basic/thermodynamicmodel.h"

namespace insight {

defineType(thermalPhaseChangeTwoPhaseSystem);
addToOpenFOAMCaseElementFactoryTable(thermalPhaseChangeTwoPhaseSystem);

thermalPhaseChangeTwoPhaseSystem::thermalPhaseChangeTwoPhaseSystem(OpenFOAMCase &c, ParameterSetInput ip)
    : thermodynamicModel(c, ip.forward<Parameters>())
{}

void thermalPhaseChangeTwoPhaseSystem::addIntoDictionaries(OFdicts &dictionaries) const
{
    //OFDictData::dict& g=dictionaries.lookupDict("constant/g");
}


bool thermalPhaseChangeTwoPhaseSystem::isUnique() const
{
    return false;
}

} // namespace insight
