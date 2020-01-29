#include "buoyantpimplefoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(buoyantPimpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(buoyantPimpleFoamNumerics);


void buoyantPimpleFoamNumerics::init()
{

  if (OFversion() < 230)
    throw insight::Exception("buoyantSimpleFoamNumerics currently supports only OF >=230");

  if (p_.boussinesqApproach)
  {
    OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimKinPressure,            FieldValue({0}), volField ) );
    OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure,            FieldValue({0}), volField ) );

    OFcase().addField("alphat", FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  }
  else
  {
    OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimPressure,            FieldValue({1e5}), volField ) );
    OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,            FieldValue({1e5}), volField ) );
  }
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0, 0.0, 0.0}), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,		FieldValue({300.0}), volField ) );
}


buoyantPimpleFoamNumerics::buoyantPimpleFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p_rgh"),
  p_(ps)
{
    init();
}


void buoyantPimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=
      p_.boussinesqApproach
      ? "buoyantBoussinesqPimpleFoam"
      : "buoyantPimpleFoam"
        ;
//  controlDict["maxCo"]=p_.maxCo;
//  controlDict["maxDeltaT"]=p_.maxDeltaT;

  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalCellLimitedGrad.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalBlendedBy.so\"" );
  controlDict.getList("libs").insertNoDuplicate( "\"libleastSquares2.so\"" );

  OFDictData::dict fqmc;
  fqmc["type"]="faceQualityMarker";
  controlDict.subDict("functions")["faceQualityMarker"]=fqmc;

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["\"rho.*\""]=OFcase().stdSymmSolverSetup(1e-8, 0.01);

  std::vector<std::string> rf({"U", "k", "epsilon", "nuTilda"});
  if (p_.boussinesqApproach)
  {
    rf.push_back("T");
  }
  else
  {
    rf.push_back("h");
  }

  for (const auto& f: rf)
  {
    solvers[f]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
    solvers[f+"Final"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  }
  solvers["p_rgh"]=OFcase().stdSymmSolverSetup(1e-8, 0.01);
  solvers["p_rghFinal"]=OFcase().stdSymmSolverSetup(1e-8, 0.0);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.01, 1);
  solvers["omegaFinal"]=OFcase().stdAsymmSolverSetup(1e-12, 0.0, 1);


  CompressiblePIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);


  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref;
  if (OFversion()>=220) pref="bounded ";

  div["default"]="none";

  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV limitedGrad";
  div["div(phi,k)"]	=	pref+"Gauss linearUpwind grad(k)";

  if (p_.boussinesqApproach)
  {
    div["div(phi,T)"]	=	pref+"Gauss linearUpwind grad(T)";
  }
  else
  {
    div["div(phi,h)"]	=	pref+"Gauss linearUpwind limitedGrad";
    div["div(phi,K)"]	=	pref+"Gauss linearUpwind limitedGrad";
  }

  div["div(phi,omega)"]	=	pref+"Gauss linearUpwind grad(omega)";
  div["div(phi,nuTilda)"]=	pref+"Gauss linearUpwind grad(nuTilda)";
  div["div(phi,epsilon)"]=	pref+"Gauss linearUpwind grad(epsilon)";
  div["div(phi,R)"]	=	pref+"Gauss upwind";
  div["div(R)"]="Gauss linear";

  if (p_.boussinesqApproach)
  {
    div["div((nuEff*dev2(T(grad(U)))))"]="Gauss linear"; // kOmegaSST2
  }
  else
  {
    div["div(((rho*nuEff)*dev(grad(U).T())))"]="Gauss linear"; // kOmegaSST2
    if (OFversion()>=300)
    {
      div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
      div["div(((rho*nu)*dev2(T(grad(U)))))"]="Gauss linear"; // LRR
    }
    else
    {
      div["div(((rho*nuEff)*dev(T(grad(U)))))"]="Gauss linear";
    }
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p_rgh"]="";
}


bool buoyantPimpleFoamNumerics::isCompressible() const
{
  if (p_.boussinesqApproach)
    return false;
  else
    return true;
}


} // namespace insight
