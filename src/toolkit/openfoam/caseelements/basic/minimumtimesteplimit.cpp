#include "minimumtimesteplimit.h"

#include "openfoam/openfoamdict.h"
#include "openfoam/ofdicts.h"

namespace insight {

defineType(minimumTimestepLimit);
addToOpenFOAMCaseElementFactoryTable(minimumTimestepLimit);

minimumTimestepLimit::minimumTimestepLimit( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "minimumTimestepLimit", ps),
  p_(ps)
{
}

void minimumTimestepLimit::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict Fd, cd, mindtd;

  Fd["type"]="runTimeControl";
  mindtd["type"]="minTimeStep";
  mindtd["minValue"]=p_.minDT;
  cd["mindt"]=mindtd;
  Fd["conditions"]=cd;

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.subDict("functions")["minimumTimestepLimit"]=Fd;

  controlDict.getList("libs").insertNoDuplicate( "\"libutilityFunctionObjects.so\"" );
}


std::string minimumTimestepLimit::category()
{
  return "Body Force";
}


} // namespace insight
