#include "interfoamnumerics.h"

#include "openfoam/ofdicts.h"
#include "openfoam/openfoamdict.h"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

namespace insight {

OFDictData::dict stdMULESSolverSetup(
    double cAlpha,
    double icAlpha,
    double tol,
    double reltol,
    bool LTS,
    int nLimiterIter )
{
  OFDictData::dict d;

  d["nAlphaCorr"]=3;
  d["nAlphaSubCycles"]=1;
  d["cAlpha"]=cAlpha;
  d["icAlpha"]=icAlpha;

  d["MULESCorr"]=true;
  d["nLimiterIter"]=nLimiterIter;
  d["alphaApplyPrevCorr"]=LTS;

  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;
  d["maxIter"]=100;

  return d;
}



defineType(interFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(interFoamNumerics);


interFoamNumerics::interFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics( c, ps, c.OFversion()<170 ? "pd" : "p_rgh" ),
  p_(ps)
{
  OFcase().setRequiredMapMethod(OpenFOAMCase::cellVolumeWeightMapMethod);

  alphaname_="alpha1";
  if (OFversion()>=230)
    alphaname_="alpha.phase1";

  // create pressure field to enable mapping from single phase cases
  OFcase().addField("p", 	FieldInfo(scalarField, dimPressure, FieldValue({0.0}), 		volField ) );

  OFcase().addField("U", 	FieldInfo(vectorField, dimVelocity, FieldValue({p_.Uinternal(0),p_.Uinternal(1),p_.Uinternal(2)}), volField ) );
  OFcase().addField(pName_, 	FieldInfo(scalarField, dimPressure, FieldValue({p_.pinternal}), volField ) );
  OFcase().addField(alphaname_,	FieldInfo(scalarField, dimless,     FieldValue({p_.alphainternal}), volField ) );
}


//const double cAlpha=0.25; // use low compression by default, since split of interface at boundaries of refinement zones otherwise
//const double icAlpha=0.1;

void interFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  if (OFversion()<06000 && OFcase().findElements<dynamicMesh>().size()>0)
   setApplicationName(dictionaries, "interDyMFoam");
  else
   setApplicationName(dictionaries, "interFoam");

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.getList("libs");
  controlDict.subDict("functions");

  controlDict.getList("libs").insertNoDuplicate( "\"liblocalFaceLimitedGrad.so\"" );


  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  for
  (
    auto f:
    std::map<std::string,double>(
     {{"", 1.}, {"Final", 0.}}
    )
  )
  {
    solvers["pcorr"+f.first]=OFcase().stdSymmSolverSetup(1e-3, 0.01*f.second);
//    if (OFversion()>=600)
//    {
//      solvers["pcorrFinal"]=stdSymmSolverSetup(1e-7, 0.01);
//    }
    solvers[pName_+f.first]=
        isGAMGOk()?
          OFcase().GAMGPCGSolverSetup(1e-7, 0.01*f.second)
        :
          OFcase().stdSymmSolverSetup(1e-7, 0.01*f.second);

    solvers["U"+f.first]=OFcase().smoothSolverSetup(1e-8, 0.1*f.second);
    solvers["k"+f.first]=OFcase().smoothSolverSetup(1e-8, 0.1*f.second);
    solvers["omega"+f.first]=OFcase().smoothSolverSetup(1e-12, 0.1*f.second, 1);
    solvers["epsilon"+f.first]=OFcase().smoothSolverSetup(1e-8, 0.1*f.second);
    solvers["nuTilda"+f.first]=OFcase().smoothSolverSetup(1e-8, 0.1*f.second);
  }

  {
   OFDictData::dict asd=stdMULESSolverSetup(
         p_.cAlpha,
         p_.icAlpha,

         1e-8,
         0.0,
         false,
         p_.alphaLimiterIter
         );
   asd["nAlphaSubCycles"]=p_.alphaSubCycles;
   solvers["\"alpha.*\""]=asd;
  }


  MultiphasePIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";

  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["grad("+alphaname_+")"]="localFaceLimited "+lqGradSchemeIfPossible()+" UBlendingFactor";

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["div(rho*phi,U)"]		= "Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
  div["div(rhoPhi,U)"]		= "Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
//  div["div(phi,alpha)"]		= "Gauss vanLeer";
//  div["div(phirb,alpha)"]	= "Gauss linear";
  div["div(phi,alpha)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(alpha)");
  div["div(phirb,alpha)"]	= "Gauss linear";
  div["div(phi,k)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
  div["div(phi,epsilon)"]	= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
  div["div(phi,omega)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
  div["div(phi,nuTilda)"]	= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
  div["div(phi,R)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(R)");
  div["div(R)"]			= "Gauss linear";
  if (OFversion()>=300)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div((muEff*dev(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["laplacian(rAUf,pcorr)"] = boost::str(boost::format("Gauss linear limited %g") % p_.snGradLowQualityLimiterReduction );
  laplacian["laplacian((1|A(U)),pcorr)"] = boost::str(boost::format("Gauss linear limited %g") % p_.snGradLowQualityLimiterReduction );
//  laplacian["default"] = boost::str(boost::format("Gauss linear localLimited UBlendingFactor %g") % p_.snGradLowQualityLimiterReduction );

//  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
////   interpolation["interpolate(U)"]="pointLinear";
////   interpolation["interpolate(HbyA)"]="pointLinear";
//  interpolation["default"]="linear"; //"pointLinear"; // OF23x: pointLinear as default creates artifacts at parallel domain borders!

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]=boost::str(boost::format("localLimited UBlendingFactor %g") % p_.snGradLowQualityLimiterReduction );

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
//  fluxRequired["default"]="no";
//  fluxRequired[pname_]="";
//  fluxRequired["alpha"]="";
  fluxRequired["pcorr"]="";
  fluxRequired[alphaname_]="";
}


bool interFoamNumerics::isCompressible() const
{
  return false;
}


} // namespace insight
