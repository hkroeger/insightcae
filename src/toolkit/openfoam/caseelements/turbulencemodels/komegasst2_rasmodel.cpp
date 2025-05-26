#include "komegasst2_rasmodel.h"

namespace insight {


defineType(kOmegaSST2_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST2_RASModel);
addToOpenFOAMCaseElementFactoryTable(kOmegaSST2_RASModel);

// void kOmegaSST2_RASModel::addFields( OpenFOAMCase& c ) const
// {
//   c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
// }

kOmegaSST2_RASModel::kOmegaSST2_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: kOmegaSST_RASModel(c, ip.forward<Parameters>())
{
//   kOmegaSST2_RASModel::addFields();
}


void kOmegaSST2_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]="kOmegaSST2";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict("kOmegaSST2");

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libkOmegaSST2.so\"") );
}

bool kOmegaSST2_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (roughness_z0>0.)
        throw insight::Exception("kOmegaSST2_RASModel: non-smooth walls are not supported!");

    if (fieldname == "k")
  {
    BC["type"]="kqRWallFunction";
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]="hybridOmegaWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["tw"]=0.057;
    BC["value"]=OFDictData::toUniformField(1.);
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]="nutHybridWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["value"]=OFDictData::toUniformField(1e-10);
    return true;
  }
  return false;
}


} // namespace insight
