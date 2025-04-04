#include "rangeweightfield.h"


#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"


namespace insight {

defineType(rangeWeightField);
addToOpenFOAMCaseElementFactoryTable(rangeWeightField);

rangeWeightField::rangeWeightField( OpenFOAMCase& c, ParameterSetInput ip )
    : OpenFOAMCaseElement(c, ip.forward<Parameters>())
{}

void rangeWeightField::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& cd=dictionaries.lookupDict("system/controlDict");
  cd.getList("libs").insertNoDuplicate("\"librangeWeightField.so\"");

  OFDictData::dict fod;
  fod["type"]="rangeWeightField";

  fod["sourceFieldName"]=p().sourceFieldName;
  fod["outputFieldName"]=p().outputFieldName;
  fod["min"]=p().min;
  fod["max"]=p().max;

  if (!p().multiplyFieldName.empty())
    fod["multiplyFieldName"]=p().multiplyFieldName;

  cd.subDict("functions")[p().outputFieldName+"_functionObject"] = fod;
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

