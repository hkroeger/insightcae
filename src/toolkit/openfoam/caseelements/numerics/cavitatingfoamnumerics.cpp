#include "cavitatingfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(cavitatingFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(cavitatingFoamNumerics);


cavitatingFoamNumerics::cavitatingFoamNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: FVNumerics(c, ip.forward<Parameters>(), "p")
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 		FieldValue({p().pamb}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0,0.0,0.0}), volField ) );
  OFcase().addField("rho", FieldInfo(scalarField, 	dimDensity, 		FieldValue({p().rhoamb}), volField ) );
}


void cavitatingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p().solverName;
  controlDict["maxCo"]=0.5;
  controlDict["maxAcousticCo"]=50.;

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=OFcase().stdSymmSolverSetup(1e-7, 0.0);
  solvers["pFinal"]=OFcase().stdSymmSolverSetup(1e-7, 0.0);
  solvers["rho"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0, 1);
  solvers["epsilon"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTilda"]=OFcase().stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& SIMPLE=fvSolution.subDict("PISO");
  SIMPLE["nCorrectors"]=OFDictData::data( 2 );
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 0 );

  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
//  std::string bgrads="Gauss linear";
//  if (OFversion()>=220) bgrads="pointCellsLeastSquares";
//  grad["default"]=bgrads;
//  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="Gauss upwind";
  div["div(phiv,rho)"]="Gauss limitedLinear 0.5";
  div["div(phi,U)"]="Gauss limitedLinearV 0.5";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
  fluxRequired["rho"]="";
}


bool cavitatingFoamNumerics::isCompressible() const
{
  return false;
}


} // namespace insight
