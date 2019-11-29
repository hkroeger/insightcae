#include "rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {

defineType(RASModel);

RASModel::RASModel(OpenFOAMCase& c, const ParameterSet& ps)
: turbulenceModel(c, ps)
{
}


void RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.lookupDict("constant/turbulenceProperties");
  if (OFversion()>=300)
    turbProperties["simulationType"]="RAS";
  else
    turbProperties["simulationType"]="RASModel";
}

OFDictData::dict& RASModel::modelPropsDict(OFdicts& dictionaries) const
{
  if (OFversion()>=300)
  {
      OFDictData::dict& turbProperties=dictionaries.lookupDict("constant/turbulenceProperties");
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
