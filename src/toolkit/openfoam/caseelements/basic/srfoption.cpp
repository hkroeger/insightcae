#include "srfoption.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(SRFoption);
addToOpenFOAMCaseElementFactoryTable(SRFoption);

SRFoption::SRFoption( OpenFOAMCase& c, ParameterSetInput ip )
: OpenFOAMCaseElement(c, ip.forward<Parameters>())
{
    // name_="SRFoption";
}



void SRFoption::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate( "\"libSRFoption.so\"" );

  OFDictData::dict cd;
  cd["type"]="SRFoption";
  OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
  fvOptions[name()]=cd;

  OFDictData::dict& SRFProperties=dictionaries.lookupDict("constant/SRFProperties");
  SRFProperties["SRFModel"]="rpm";
  SRFProperties["origin"]=OFDictData::vector3(p().origin);
  SRFProperties["axis"]=OFDictData::vector3( p().axis/arma::norm(p().axis,2) );

  OFDictData::dict& rpmCoeffs=SRFProperties.subDict("rpmCoeffs");
  rpmCoeffs["rpm"]=p().rpm;
}

} // namespace insight
