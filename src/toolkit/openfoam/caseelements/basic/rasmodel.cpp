#include "rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(RASModel);

RASModel::RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: turbulenceModel(c, ip.forward<Parameters>())
{}


std::string RASModel::RASModelName() const
{
  return type();
}


OFDictData::dict RASModel::RASModelCoeffs() const
{
  OFDictData::dict d;
  d["Prt"]=p().Prt;
  return d;
}


void RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.lookupDict(turbPropertiesDictName());
  if (OFversion()>=300)
    turbProperties["simulationType"]="RAS";
  else
    turbProperties["simulationType"]="RASModel";

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  std::string modelName=RASModelName();
  RASProperties["RASModel"]=modelName;
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict(modelName+"Coeffs").merge_overwrite(RASModelCoeffs());
}

OFDictData::dict& RASModel::modelPropsDict(OFdicts& dictionaries) const
{
  if (OFversion()>=300)
  {
      OFDictData::dict& turbProperties=dictionaries.lookupDict(turbPropertiesDictName());
      return turbProperties.subDict("RAS");
  }
  else
  {
      return dictionaries.lookupDict("constant/RASProperties");
  }
}

turbulenceModel::AccuracyRequirement RASModel::minAccuracyRequirement() const
{
  return AC_RANS;
}

} // namespace insight
