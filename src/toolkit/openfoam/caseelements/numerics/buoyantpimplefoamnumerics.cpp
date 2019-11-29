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

  OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimPressure,            FieldValue({1e5}), volField ) );
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,            FieldValue({1e5}), volField ) );
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
  controlDict["application"]="buoyantPimpleFoam";
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

  solvers["p_rgh"]=OFcase().stdSymmSolverSetup(1e-8, 0.01);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["h"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.01, 1);
  solvers["epsilon"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["nuTilda"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);

  solvers["p_rghFinal"]=OFcase().stdSymmSolverSetup(1e-8, 0.0);
  solvers["UFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["hFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["omegaFinal"]=OFcase().stdAsymmSolverSetup(1e-12, 0.0, 1);
  solvers["epsilonFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["nuTildaFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);


//  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
//  if (p_.nOuterCorrectors>1)
//    {
//      if (OFversion()<210)
//      {
//        relax["p_rgh"]=0.3;
//        relax["U"]=0.7;
//        relax["k"]=0.7;
//        relax["R"]=0.7;
//        relax["omega"]=0.7;
//        relax["epsilon"]=0.7;
//        relax["nuTilda"]=0.7;
//      }
//      else
//      {
//        OFDictData::dict fieldRelax, eqnRelax;
//        fieldRelax["p_rgh"]=0.3;
//        eqnRelax["U"]=0.7;
//        eqnRelax["k"]=0.7;
//        eqnRelax["R"]=0.7;
//        eqnRelax["omega"]=0.7;
//        eqnRelax["epsilon"]=0.7;
//        eqnRelax["nuTilda"]=0.7;
//        relax["fields"]=fieldRelax;
//        relax["equations"]=eqnRelax;
//      }
//    }

//  // create both: PISO and PIMPLE
////   if (LES)
//  {
//    OFDictData::dict& PISO=fvSolution.subDict("PISO");
//    PISO["nCorrectors"]=p_.nCorrectors;
//    PISO["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;
//    PISO["pRefCell"]=0;
//    PISO["pRefValue"]=0.0;
//  }
////   else
//  {
//    OFDictData::dict& PIMPLE=fvSolution.subDict("PIMPLE");
//    PIMPLE["nCorrectors"]=p_.nCorrectors;
//    PIMPLE["nOuterCorrectors"]=p_.nOuterCorrectors;
//    PIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;
//    PIMPLE["pRefCell"]=0;
//    PIMPLE["pRefValue"]=0.0;
//    PIMPLE["momentumPredictor"]=false;
//    PIMPLE["rhoMin"]=0.01;
//    PIMPLE["rhoMax"]=100.;
//  }

  CompressiblePIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);


  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

//  std::string bgrads="Gauss linear";
//  if ( (OFversion()>=220) && !p_.hasCyclics ) bgrads="pointCellsLeastSquares";

//  grad["default"]=bgrads;
//  grad["grad(p_rgh)"]="Gauss linear";
  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref;
  if (OFversion()>=220) pref="bounded ";
  div["default"]="none";
  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV limitedGrad";
  div["div(phi,K)"]	=	pref+"Gauss linearUpwind limitedGrad";
  div["div(phi,k)"]	=	pref+"Gauss linearUpwind grad(k)";
  div["div(phi,h)"]	=	pref+"Gauss linearUpwind limitedGrad";
  div["div(phi,omega)"]	=	pref+"Gauss linearUpwind grad(omega)";
  div["div(phi,nuTilda)"]=	pref+"Gauss linearUpwind grad(nuTilda)";
  div["div(phi,epsilon)"]=	pref+"Gauss linearUpwind grad(epsilon)";
  div["div(phi,R)"]	=	pref+"Gauss upwind";
  div["div(R)"]="Gauss linear";

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
  return true;
}


} // namespace insight
