#include "dynsmagorinsky_lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(dynSmagorinsky_LESModel);
addToFactoryTable(turbulenceModel, dynSmagorinsky_LESModel);

addToOpenFOAMCaseElementFactoryTable(dynSmagorinsky_LESModel);

void dynSmagorinsky_LESModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  if (c.OFversion()>=300)
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}

dynSmagorinsky_LESModel::dynSmagorinsky_LESModel(OpenFOAMCase& c, ParameterSetInput ip)
: LESModel(c, ip.forward<Parameters>())
{
//   addFields();
}



void dynSmagorinsky_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& LESProperties=modelPropsDict(dictionaries);

  std::string modelName="dynSmagorinsky";
  if (OFversion()>=170)
    modelName="homogeneousDynSmagorinsky";

  LESProperties["LESModel"]=modelName;
  LESProperties["turbulence"]="true";
  //LESProperties["delta"]="cubeRootVol";
  LESProperties["delta"]="vanDriest";
  LESProperties["printCoeffs"]=true;

  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;

  OFDictData::dict vdc;
  vdc["deltaCoeff"]=1.0;
  vdc["delta"]="cubeRootVol";
  vdc["cubeRootVolCoeffs"]=crvc;
  LESProperties["vanDriestCoeffs"]=vdc;

  OFDictData::dict& cd=LESProperties.subDict(modelName+"Coeffs");
  cd["filter"]="simple";
}


} // namespace insight
