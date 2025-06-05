#include "komegahe_rasmodel.h"

using namespace boost;

namespace insight {


defineType(kOmegaHe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaHe_RASModel);
addToOpenFOAMCaseElementFactoryTable(kOmegaHe_RASModel);

kOmegaHe_RASModel::kOmegaHe_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: kOmegaSST_RASModel(c, ip.forward<Parameters>())
{}


void kOmegaHe_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libkOmegaHe.so\"") );

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]="kOmegaHe";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict("kOmegaHeCoeffs");

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["div(nonlinear)"]="Gauss linear"; //pref+"Gauss upwind";
  div["div((nuEff*T(grad(U))))"]="Gauss linear";

}

bool kOmegaHe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (roughness_z0>0.)
        throw insight::Exception("kOmegaHe_RASModel: non-smooth walls are not supported!");

  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  else if ( fieldname == "omega")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  else if ( fieldname == "nut")
  {
    BC["type"]=OFDictData::data("calculated");
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  return false;
}


} // namespace insight
