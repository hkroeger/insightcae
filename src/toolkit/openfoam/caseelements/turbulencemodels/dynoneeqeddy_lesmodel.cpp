#include "dynoneeqeddy_lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(dynOneEqEddy_LESModel);
addToFactoryTable(turbulenceModel, dynOneEqEddy_LESModel);

addToOpenFOAMCaseElementFactoryTable(dynOneEqEddy_LESModel);

void dynOneEqEddy_LESModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({1e-10}), volField ) );
  if (c.OFversion()>=300)
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}


dynOneEqEddy_LESModel::dynOneEqEddy_LESModel(OpenFOAMCase& c, const ParameterSet& ps)
: LESModel(c)
{
//   addFields();
}


void dynOneEqEddy_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);


  OFDictData::dict& LESProperties=modelPropsDict(dictionaries);
  LESProperties["printCoeffs"]=true;

  std::string modelname="dynOneEqEddy";
  if (OFversion()>=300) modelname="dynamicKEqn";

  LESProperties["LESModel"]=modelname;
  LESProperties["turbulence"]="true";
  LESProperties["delta"]="cubeRootVol";
//   LESProperties["delta"]="vanDriest";

  OFDictData::dict doeec;
  doeec["filter"]="simple";
  LESProperties[modelname+"Coeffs"]=doeec;

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
