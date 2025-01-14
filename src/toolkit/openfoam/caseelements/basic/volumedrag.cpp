#include "volumedrag.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"

namespace insight {

defineType(volumeDrag);
addToOpenFOAMCaseElementFactoryTable(volumeDrag);

volumeDrag::volumeDrag( OpenFOAMCase& c, ParameterSetInput ip )
: cellSetFvOption(c, /*"volumeDrag"+ps.getString("name"),*/
                      ip.forward<Parameters>())
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
  vdd["cellZone"]=p().name;
  vdd["selectionMode"]="cellZone";
  vdd["CD"]=OFDictData::to_OF(p().CD);
  cd["volumeDragCoeffs"]=vdd;

  fvOptions[name()]=cd;
  cellSetFvOption::addIntoFvOptionDictionary(fvOptions, dictionaries);
}


} // namespace insight
