#include "wale_lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(WALE_LESModel);
addToFactoryTable(turbulenceModel, WALE_LESModel);
addToOpenFOAMCaseElementFactoryTable(WALE_LESModel);

void WALE_LESModel::addFields(OpenFOAMCase& c) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  if (c.OFversion()>=300)
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}

WALE_LESModel::WALE_LESModel(OpenFOAMCase& c, ParameterSetInput ip)
: LESModel(c, ip.forward<Parameters>())
{
//   addFields();
}


void WALE_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs").insertNoDuplicate("\"libdynamicMixedModelLESModel.so\"");

//   OFDictData::dict& LESProperties=dictionaries.lookupDict("constant/LESProperties");
  OFDictData::dict& LESProperties=modelPropsDict(dictionaries);

  std::string modelName="WALE";

  LESProperties["LESModel"]=modelName;
  LESProperties["delta"]="cubeRootVol";
//   LESProperties["delta"]="vanDriest";
  LESProperties["printCoeffs"]=true;

  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;

//   OFDictData::dict& cd=LESProperties.subDict(modelName+"Coeffs");
//   cd["filter"]="simple";
}


} // namespace insight
