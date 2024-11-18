#include "volumedrag.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(volumeDrag);
addToOpenFOAMCaseElementFactoryTable(volumeDrag);

volumeDrag::volumeDrag( OpenFOAMCase& c, const ParameterSet& ps )
: cellSetFvOption(c, "volumeDrag"+ps.getString("name"), ps),
  p_(ps)
{
}


void volumeDrag::addIntoFvOptionDictionary(
    OFDictData::dict& fvOptions,
    OFdicts& dictionaries ) const
{
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate( "\"libvolumeDragfvOption.so\"" );

  OFDictData::dict cd;
  cd["type"]="volumeDrag";
  OFDictData::dict vdd;
  vdd["cellZone"]=p_.name;
  vdd["selectionMode"]="cellZone";
  vdd["CD"]=OFDictData::to_OF(p_.CD);
  cd["volumeDragCoeffs"]=vdd;

  fvOptions[name()]=cd;
  cellSetFvOption::addIntoFvOptionDictionary(fvOptions, dictionaries);
}


} // namespace insight
