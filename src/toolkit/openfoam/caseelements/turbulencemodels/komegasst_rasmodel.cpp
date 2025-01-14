#include "komegasst_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(kOmegaSST_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_RASModel);

addToOpenFOAMCaseElementFactoryTable(kOmegaSST_RASModel);

void kOmegaSST_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  c.addField("omega", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 0, -1), 	FieldValue({1.0}), volField ) );
  if (c.isCompressible())
  {
    c.addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
  }

  if (c.isCompressible() && (c.OFversion()<300))
    c.addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}

kOmegaSST_RASModel::kOmegaSST_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: RASModel(c, ip.forward<Parameters>())
{
//   addFields();
}


void kOmegaSST_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  if (const auto *none = boost::get<Parameters::freeSurfaceProductionDamping_none_type>(&p().freeSurfaceProductionDamping))
  {
    RASProperties["RASModel"]="kOmegaSST";
    RASProperties.subDict("kOmegaSSTCoeffs");
  }
  else if (const auto *damp = boost::get<Parameters::freeSurfaceProductionDamping_enabled_type>(&p().freeSurfaceProductionDamping))
  {
    RASProperties["RASModel"]="kOmegaSST3";

    auto& coeffs = RASProperties.subDict("kOmegaSST3Coeffs");
    coeffs["freeSurfaceAlphaName"]=damp->alphaFieldName;

    OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
    controlDict.getList("libs").insertNoDuplicate( "\"libkOmegaSST3.so\"" );
  }
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
}

bool kOmegaSST_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
  std::string turbpref="", pref="";
  if (OFcase().isCompressible() )
    {

      if ( (OFversion()<164) || (OFversion()>170) )
        pref="compressible::";

      if (OFcase().OFversion()<300)
        turbpref="compressible::";

    }

  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data(turbpref+"kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]=OFDictData::data(turbpref+"omegaWallFunction");
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["beta1"]=0.075;
    BC["blended"]=true;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
    if (OFversion()>=164)
    {
      if (roughness_z0>0.)
      {
          BC["type"]=turbpref+"nutURoughWallFunction";
          double Cs=0.5;
          BC["roughnessConstant"]=Cs;
          BC["roughnessHeight"]=roughness_z0*9.793/Cs;
          BC["roughnessFactor"]=1.0;
          BC["value"]="uniform 1e-10";
      }
      else
      {
        BC["type"]=turbpref+"nutUWallFunction";
        BC["value"]=OFDictData::data("uniform 1e-10");
      }
    }
    else
    {
      BC["type"]=turbpref+"nutWallFunction";
      BC["value"]=OFDictData::data("uniform 1e-10");
    }
    return true;
  }
  else if (fieldname == "mut")
  {
    BC["type"]=OFDictData::data("mutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "alphat")
  {
    BC["type"]=pref+"alphatWallFunction";
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }

  return false;
}

} // namespace insight
