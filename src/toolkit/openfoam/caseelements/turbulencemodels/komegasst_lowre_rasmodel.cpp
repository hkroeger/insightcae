#include "komegasst_lowre_rasmodel.h"

using namespace boost;

namespace insight {


defineType(kOmegaSST_LowRe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_LowRe_RASModel);
addToOpenFOAMCaseElementFactoryTable(kOmegaSST_LowRe_RASModel);

kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(OpenFOAMCase& c, const ParameterSet& ps)
: kOmegaSST_RASModel(c)
{}


void kOmegaSST_LowRe_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]="kOmegaSST_LowRe";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict("kOmegaSST_LowReCoeffs");
}

bool kOmegaSST_LowRe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (roughness_z0>0.)
        throw insight::Exception("kOmegaSST_LowRe_RASModel: non-smooth walls are not supported!");

    if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  else if ( fieldname == "omega")
  {
    BC["type"]=OFDictData::data("omegaWallFunction");
    BC["blended"]=true;
    BC["value"]="uniform "+str(format("%g") % 1e-10);
    return true;
  }
  return false;
}


} // namespace insight
