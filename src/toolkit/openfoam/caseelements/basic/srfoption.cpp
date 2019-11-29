#include "srfoption.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {


defineType(SRFoption);
addToOpenFOAMCaseElementFactoryTable(SRFoption);

SRFoption::SRFoption( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="SRFoption";
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
  SRFProperties["origin"]=OFDictData::vector3(p_.origin);
  SRFProperties["axis"]=OFDictData::vector3( p_.axis/arma::norm(p_.axis,2) );

  OFDictData::dict& rpmCoeffs=SRFProperties.subDict("rpmCoeffs");
  rpmCoeffs["rpm"]=p_.rpm;
}

} // namespace insight
