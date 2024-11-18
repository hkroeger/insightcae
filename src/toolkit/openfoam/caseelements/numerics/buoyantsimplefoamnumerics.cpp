#include "buoyantsimplefoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

namespace insight {


defineType(buoyantSimpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(buoyantSimpleFoamNumerics);


void buoyantSimpleFoamNumerics::init()
{

  if (OFversion() < 230)
    throw insight::UnsupportedFeature("buoyantSimpleFoamNumerics currently supports only OF >=230");

  if (p_.boussinesqApproach)
  {
      OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimKinPressure,            FieldValue({p_.pinternal}), volField ) );
      OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure,            FieldValue({p_.pinternal}), volField ) );
      OFcase().addField("alphat", FieldInfo(scalarField, 	dimKinViscosity, 	FieldValue({1e-10}), volField ) );
  }
  else
  {
      OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimPressure,            FieldValue({p_.pinternal}), volField ) );
      OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,            FieldValue({p_.pinternal}), volField ) );
  }
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0, 0.0, 0.0}), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,		FieldValue({p_.Tinternal}), volField ) );
}


buoyantSimpleFoamNumerics::buoyantSimpleFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p_rgh"),
  p_(ps)
{
    init();
}


void buoyantSimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=
      p_.boussinesqApproach
          ? "buoyantBoussinesqSimpleFoam"
          : "buoyantSimpleFoam"
      ;

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

  std::string energy("h");
  if (p_.boussinesqApproach)
  {
      energy="T";
  }

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p_rgh"]=OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers[energy]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.01, 1);
  solvers["epsilon"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["nuTilda"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);


  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["p_rgh"]=0.7;
    relax["rho"]=0.9;
    relax["U"]=0.2;
    relax["k"]=0.7;
    relax[energy]=0.2;
    relax["R"]=0.7;
    relax["omega"]=0.7;
    relax["epsilon"]=0.7;
    relax["nuTilda"]=0.7;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p_rgh"]=0.7;
    fieldRelax["rho"]=0.9;
    eqnRelax["U"]=0.2;
    eqnRelax["k"]=0.7;
    eqnRelax[energy]=0.2;
    eqnRelax["R"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    eqnRelax["nuTilda"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;
  SIMPLE["momentumPredictor"]=false;
  SIMPLE["pRefCell"]=0;
  SIMPLE["pRefValue"]=p_.pinternal;

  if ( (OFversion()>=210) && p_.checkResiduals )
  {
    OFDictData::dict resCtrl;
    resCtrl["p_rgh"]=1e-4;
    resCtrl["U"]=1e-3;
    resCtrl["\"(k|epsilon|omega|nuTilda|R)\""]=1e-4;
    SIMPLE["residualControl"]=resCtrl;
  }

  // ============ setup fvSchemes ================================

  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

//  std::string bgrads="Gauss linear";
//  if ( (OFversion()>=220) && !p_.hasCyclics ) bgrads="pointCellsLeastSquares";

//  grad["default"]=bgrads;
//  grad["grad(p_rgh)"]="Gauss linear";
  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref;
  if (OFversion()>=220) pref="bounded ";
//  suf="localCellLimited "+bgrads+" UBlendingFactor";
//  if (OFversion()>=170)
//  {
//    grad["limitedGrad"]=suf;
//    suf="limitedGrad";
//  }
  div["default"]="none";
  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV limitedGrad";
  div["div(phi,k)"]	=	pref+"Gauss linearUpwind grad(k)";
  if (p_.boussinesqApproach)
  {
      div["div(phi,T)"]	=	pref+"Gauss linearUpwind grad(T)";
  }
  else
  {
      div["div(phi,K)"]	=	pref+"Gauss linearUpwind limitedGrad";
      div["div(phi,h)"]	=	pref+"Gauss linearUpwind limitedGrad";
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


bool buoyantSimpleFoamNumerics::isCompressible() const
{
    if (p_.boussinesqApproach)
        return false;
    else
        return true;
}



} // namespace insight
