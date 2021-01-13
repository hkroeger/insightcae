#include "volumedrag.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(volumeDrag);
addToOpenFOAMCaseElementFactoryTable(volumeDrag);

volumeDrag::volumeDrag( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "", ps),
  p_(ps)
{
    name_="volumeDrag"+p_.name;
}


void volumeDrag::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );

  OFDictData::dict cd;
  cd["type"]="volumeDrag";
  cd["active"]=true;
  OFDictData::dict vdd;
  vdd["cellZone"]=p_.name;
  vdd["selectionMode"]="cellZone";
  vdd["CD"]=OFDictData::to_OF(p_.CD);
  cd["volumeDragCoeffs"]=vdd;

  OFDictData::dict& fvOptions=dictionaries.lookupDict("system/fvOptions");
  fvOptions[p_.name]=cd;
}


} // namespace insight
