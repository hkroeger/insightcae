#include "lemoshybrid_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(LEMOSHybrid_RASModel);
addToFactoryTable(turbulenceModel, LEMOSHybrid_RASModel);
addToOpenFOAMCaseElementFactoryTable(LEMOSHybrid_RASModel);

void LEMOSHybrid_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField(fieldName("kSgs"),    FieldInfo(scalarField, dimKinEnergy,    FieldValue({1e-10}), volField ) );
  c.addField(fieldName("nuSgs"),   FieldInfo(scalarField, dimKinViscosity, FieldValue({1e-10}), volField ) );
  c.addField(fieldName("UAvgHyb"),FieldInfo(vectorField,  dimVelocity,     FieldValue({0,0,0}), volField ) );
}

LEMOSHybrid_RASModel::LEMOSHybrid_RASModel(OpenFOAMCase& c, ParameterSetInput ip)
: kOmegaSST_RASModel(c, ip.forward<Parameters>())
{
//   addFields();
}



std::string LEMOSHybrid_RASModel::RASModelName() const
{
  return "hybKOmegaSST2";
}


OFDictData::dict LEMOSHybrid_RASModel::RASModelCoeffs() const
{
  OFDictData::dict mec;
  mec["deltaCoeff"]=1.0;

  OFDictData::dict cd;
  cd["filter"]="simple";
  cd["x1"]=1.0;
  cd["x2"]=2.0;
  cd["Cint"]=1.0;
  cd["CN"]=1.0;

  cd["averagingTime"]=1;
  cd["fixedInterface"]=false;
  cd["useIDDESDelta"]=false;

  cd["delta"]="maxEdge";

  cd["cubeRootVolCoeffs"]=mec;
  cd["IDDESDeltaCoeffs"]=mec;
  cd["maxEdgeCoeffs"]=mec;

  return cd;
}


void LEMOSHybrid_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()<230)
    throw insight::UnsupportedFeature("The LES model "+RASModelName()+" is unsupported in the selected OF version!");

  // add k-O stuff first
  kOmegaSST_RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);
  RASProperties["delta"]="maxEdge";

  OFDictData::dict mec;
  mec["deltaCoeff"]=1.0;
  RASProperties["maxEdgeCoeffs"]=mec;

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libLEMOS-2.3.x.so\"") );
}

bool LEMOSHybrid_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
  if (!kOmegaSST_RASModel::addIntoFieldDictionary(fieldname, fieldinfo, BC, roughness_z0))
  {
    if (fieldname == fieldName("kSgs"))
    {
      BC["type"]="fixedValue";
      BC["value"]=OFDictData::toUniformField(0.);
      return true;
    }
    else if (fieldname == fieldName("nuSgs"))
    {
      BC["type"]="zeroGradient";
      return true;
    }
  }

  return false;
}


} // namespace insight
