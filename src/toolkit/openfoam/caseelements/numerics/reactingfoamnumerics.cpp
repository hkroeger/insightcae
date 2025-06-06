#include "reactingfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/turbulencemodel.h"

using namespace std;

namespace insight {


defineType(reactingFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingFoamNumerics);


void reactingFoamNumerics::init()
{

  if (OFversion() < 230)
    throw insight::UnsupportedFeature("reactingFoamNumerics currently supports only OF >=230");

  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField(pName_, FieldInfo(scalarField, 	dimPressure, 	FieldValue({p().pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({p().Uinternal[0], p().Uinternal[1], p().Uinternal[2]}), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,		FieldValue({p().Tinternal}), volField ) );
}


reactingFoamNumerics::reactingFoamNumerics(OpenFOAMCase& c, ParameterSetInput ip)
: FVNumerics(c, ip.forward<Parameters>(), "p")
{
  if (p().buoyancy) pName_="p_rgh";
  init();
}


void reactingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  std::string solverName="reactingFoam";
  if (p().buoyancy)
    solverName="rhoReactingBuoyantFoam";
  controlDict["application"]=solverName;

  CompressiblePIMPLESettings ps(p().time_integration);
  ps.addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");



  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["\"rho.*\""]=OFcase().stdSymmSolverSetup(0, 0);
  solvers[pName_]=OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["h"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);

  solvers[pName_+"Final"]=OFcase().stdSymmSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["hFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=OFcase().stdAsymmSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);

  solvers["Yi"]=OFcase().stdAsymmSolverSetup(1e-8, 0);

  // ============ setup fvSchemes ================================

  // check if LES required
  bool LES=p().forceLES;
  try
  {
      auto& tm = this->OFcase().findUniqueElement<turbulenceModel>();
      LES=LES || (tm.minAccuracyRequirement() >= turbulenceModel::AC_LES);
  }
  catch (...)
  {
    std::cout<<"Warning: unhandled exception during LES check!"<<std::endl;
  }

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (LES)
    ddt["default"]="backward";
  else
    ddt["default"]="Euler";

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
//  std::string bgrads="Gauss linear";
//  if (OFversion()>=220) bgrads="pointCellsLeastSquares";
//  grad["default"]=bgrads;
//  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";

  if (ps.isSIMPLE())
  {
    // SIMPLE mode: add underrelaxation
    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OFversion()<210)
    {
      relax["Yi"]=0.7;
    }
    else
    {
      OFDictData::dict& eqnRelax = relax.subDict("equations");
      eqnRelax["Yi"]=0.7;
    }
  }

  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
      div["div(phi,U)"]="Gauss linear";

    div["div(phid,p)"]="Gauss limitedLinear 1";
    div["div(phi,k)"]="Gauss limitedLinear 1";
    div["div(phi,Yi_h)"]="Gauss limitedLinear 1";
    div["div(phi,epsilon)"]="Gauss limitedLinear 1";
    div["div(phi,omega)"]="Gauss limitedLinear 1";
    div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
  }
  else
  {
    div["div(phi,U)"]="Gauss linearUpwindV limitedGrad";
    div["div(phid,p)"]="Gauss limitedLinear 1";
    div["div(phi,k)"]="Gauss linearUpwind limitedGrad";
    div["div(phi,Yi_h)"]="Gauss upwind";
    div["div(phi,epsilon)"]="Gauss linearUpwind limitedGrad";
    div["div(phi,omega)"]="Gauss linearUpwind limitedGrad";
    div["div(phi,nuTilda)"]="Gauss linearUpwind limitedGrad";
  }

  if (OFversion()>=600)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((muEff*dev2(T(grad(U)))))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["interpolate(U)"]="pointLinear";
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired[pName_]="";
}


bool reactingFoamNumerics::isCompressible() const
{
  return true;
}


} // namespace insight
