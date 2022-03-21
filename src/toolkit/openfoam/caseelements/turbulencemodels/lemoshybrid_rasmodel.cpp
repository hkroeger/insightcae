#include "lemoshybrid_rasmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(LEMOSHybrid_RASModel);
addToFactoryTable(turbulenceModel, LEMOSHybrid_RASModel);
addToOpenFOAMCaseElementFactoryTable(LEMOSHybrid_RASModel);

void LEMOSHybrid_RASModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("kSgs", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  c.addField("UAvgHyb", 	FieldInfo(vectorField, 	dimVelocity, 	FieldValue({0,0,0}), volField ) );
}

LEMOSHybrid_RASModel::LEMOSHybrid_RASModel(OpenFOAMCase& c, const ParameterSet& ps)
: kOmegaSST_RASModel(c)
{
//   addFields();
}



void LEMOSHybrid_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  // add k-O stuff first, we will overwrite afterwards, where necessary
  kOmegaSST_RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=modelPropsDict(dictionaries);

  std::string modelName="hybKOmegaSST2";

  if (OFversion()<230)
    throw insight::UnsupportedFeature("The LES model "+modelName+" is unsupported in the selected OF version!");

  RASProperties["RASModel"]=modelName;
  RASProperties["delta"]="maxEdge";
  RASProperties["printCoeffs"]=true;

  OFDictData::dict mec;
  mec["deltaCoeff"]=1.0;
  RASProperties["maxEdgeCoeffs"]=mec;

  OFDictData::dict& cd=RASProperties.subDict(modelName+"Coeffs");
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

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libLEMOS-2.3.x.so\"") );
}

bool LEMOSHybrid_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC, double roughness_z0) const
{
  if (!kOmegaSST_RASModel::addIntoFieldDictionary(fieldname, fieldinfo, BC, roughness_z0))
  {
    if (fieldname == "kSgs")
    {
      BC["type"]="fixedValue";
      BC["value"]="uniform 0";
      return true;
    }
    else if (fieldname == "nuSgs")
    {
      BC["type"]="zeroGradient";
      return true;
    }
  }

  return false;
}


} // namespace insight
