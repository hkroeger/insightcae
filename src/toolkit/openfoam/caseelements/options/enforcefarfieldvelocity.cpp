#include "enforcefarfieldvelocity.h"

#include "openfoam/openfoamdict.h"
#include "openfoam/ofdicts.h"

namespace insight {


defineType(EnforceFarFieldVelocity);
addToOpenFOAMCaseElementFactoryTable(EnforceFarFieldVelocity);


EnforceFarFieldVelocity::EnforceFarFieldVelocity(OpenFOAMCase& c, ParameterSetInput ip)
    : OpenFOAMCaseElement(c, /*"EnforceFarFieldVelocity", */ip.forward<Parameters>())
{
}


void EnforceFarFieldVelocity::addIntoDictionaries ( OFdicts& dictionaries ) const
{
    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.getList("libs").insertNoDuplicate( "\"libenforceFarFieldVelocity.so\"" );

    OFDictData::dict coeffs;

    coeffs["type"]="enforceFarFieldVelocity";
    coeffs["farFieldVelocityFieldName"]=p().farFieldVelocityFieldName;

    OFDictData::list pns;
    std::copy(
                p().farFieldPatches.begin(), p().farFieldPatches.end(),
                std::back_inserter(pns) );
    coeffs["farFieldPatches"]=pns;
    coeffs["transitionDistance"]=p().transitionDistance;

    OFDictData::dict pd;
    pd["method"]="meshWave";
    coeffs["patchDist"]=pd;

    OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
    fvOptions[name()]=coeffs;
}

} // namespace insight
