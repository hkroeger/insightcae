#include "oneeqeddy_lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(oneEqEddy_LESModel);
addToFactoryTable(turbulenceModel, oneEqEddy_LESModel);

addToOpenFOAMCaseElementFactoryTable(oneEqEddy_LESModel);

void oneEqEddy_LESModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  if (c.OFversion()>=300)
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}


oneEqEddy_LESModel::oneEqEddy_LESModel(OpenFOAMCase& c, ParameterSetInput ip)
: LESModel(c, ip.forward<Parameters>())
{
//   addFields();
}


void oneEqEddy_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& LESProperties=modelPropsDict(dictionaries);
  LESProperties["printCoeffs"]=true;

  std::string modelname="oneEqEddy";
  if (OFversion()>=300) modelname="kEqn";

  LESProperties["LESModel"]=modelname;
  LESProperties["turbulence"]="true";
  //LESProperties["delta"]="cubeRootVol";
  LESProperties["delta"]="vanDriest";

  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;

  OFDictData::dict vdc;
  vdc["deltaCoeff"]=1.0;
  vdc["delta"]="cubeRootVol";
  vdc["cubeRootVolCoeffs"]=crvc;
  LESProperties["vanDriestCoeffs"]=vdc;

  LESProperties.subDict("laminarCoeffs");
}


} // namespace insight
