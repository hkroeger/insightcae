#include "rangeweightfield.h"


#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"


namespace insight {

defineType(rangeWeightField);
addToOpenFOAMCaseElementFactoryTable(rangeWeightField);

rangeWeightField::rangeWeightField( OpenFOAMCase& c, const ParameterSet& ps )
: OpenFOAMCaseElement(c, "rangeWeightField", ps),
  p_(ps)
{
}

void rangeWeightField::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& cd=dictionaries.lookupDict("system/controlDict");
  cd.getList("libs").insertNoDuplicate("\"librangeWeightField.so\"");

  OFDictData::dict fod;
  fod["type"]="rangeWeightField";

  fod["sourceFieldName"]=p_.sourceFieldName;
  fod["outputFieldName"]=p_.outputFieldName;
  fod["min"]=p_.min;
  fod["max"]=p_.max;

  if (!p_.multiplyFieldName.empty())
    fod["multiplyFieldName"]=p_.multiplyFieldName;

  cd.subDict("functions")[p_.outputFieldName+"_functionObject"] = fod;
}


std::string rangeWeightField::category()
{
  return "PostProcessing";
}

bool rangeWeightField::isUnique() const
{
  return true;
}

} // namespace insight

