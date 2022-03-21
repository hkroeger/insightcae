#include "smagorinsky_lesmodel.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(Smagorinsky_LESModel);
addToFactoryTable(turbulenceModel, Smagorinsky_LESModel);

addToOpenFOAMCaseElementFactoryTable(Smagorinsky_LESModel);

void Smagorinsky_LESModel::addFields( OpenFOAMCase& c ) const
{
  c.addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	FieldValue({ 1e-10 }), volField ) );
  if (c.OFversion()>=300)
    c.addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  else
    c.addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
}


Smagorinsky_LESModel::Smagorinsky_LESModel(OpenFOAMCase& c, const ParameterSet& ps)
: LESModel(c),
  p_(ps)
{
//   addFields();
}


void Smagorinsky_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& LESProperties=modelPropsDict(dictionaries);
  LESProperties["printCoeffs"]=true;

  std::string modelname="Smagorinsky";

  LESProperties["LESModel"]=modelname;
  LESProperties["turbulence"]="true";
  //LESProperties["delta"]="cubeRootVol";
  LESProperties["delta"]="vanDriest";

  OFDictData::dict smc;
  //smc["Ck"]=OFDictData::dimensionedData("Ck", dimless, p_.C);
  smc["Ck"]=p_.C;
  LESProperties[modelname+"Coeffs"]=smc;


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
