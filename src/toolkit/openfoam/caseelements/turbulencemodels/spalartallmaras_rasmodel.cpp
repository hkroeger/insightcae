#include "spalartallmaras_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(SpalartAllmaras_RASModel);
addToFactoryTable(turbulenceModel, SpalartAllmaras_RASModel);
addToOpenFOAMCaseElementFactoryTable(SpalartAllmaras_RASModel);

void SpalartAllmaras_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("nuTilda", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  if (c.isCompressible())
  {
    c.addField("mut", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
    c.addField("alphat", 	FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({1e-10}), volField ) );
  }
  else
  {
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  }
}

SpalartAllmaras_RASModel::SpalartAllmaras_RASModel(OpenFOAMCase& c, const ParameterSet& ps)
: RASModel(c)
{
//   addFields();
}


void SpalartAllmaras_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["RASModel"]="SpalartAllmaras";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.subDict("SpalartAllmarasCoeffs");
}

bool SpalartAllmaras_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
    if (roughness_z0>0.)
        throw insight::Exception("SpalartAllmaras_RASModel: non-smooth walls are not supported!");

//   std::string pref="";
//   if (OFcase().isCompressible()) pref="compressible::";

  if (fieldname == "nuTilda")
  {
    BC["type"]=OFDictData::data("fixedValue");
    BC["value"]=OFDictData::data("uniform 0");
    return true;
  }
  else if (fieldname == "nut")
  {
    if (OFversion()>=170)
    {
      BC["type"]=OFDictData::data("nutUSpaldingWallFunction");
      BC["value"]=OFDictData::data("uniform 0");
    }
    else
    {
      BC["type"]=OFDictData::data("nutSpalartAllmarasWallFunction");
      BC["value"]=OFDictData::data("uniform 0");
    }
    return true;
  }

  return false;
}


} // namespace insight
