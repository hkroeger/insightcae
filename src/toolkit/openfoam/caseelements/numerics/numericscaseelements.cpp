/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "numericscaseelements.h"
#include "openfoam/openfoamcaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"


#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;

namespace insight
{










defineType(potentialFreeSurfaceFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(potentialFreeSurfaceFoamNumerics);


potentialFreeSurfaceFoamNumerics::potentialFreeSurfaceFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: unsteadyIncompressibleNumerics(c, ps, "p_gh"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({0.0}), volField ) );
//  OFcase().addField("p_gh", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({0.0}), volField ) );
//  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0,0.0,0.0}), volField ) );
  
  if (OFversion()<230)
    throw insight::Exception("solver potentialFreeSurfaceFoam not available in selected OpenFOAM version!");
}


void potentialFreeSurfaceFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  unsteadyIncompressibleNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  if (OFcase().findElements<dynamicMesh>().size()>0)
   setApplicationName(dictionaries, "potentialFreeSurfaceDyMFoam");
  else
   setApplicationName(dictionaries, "potentialFreeSurfaceFoam");

}


bool potentialFreeSurfaceFoamNumerics::isCompressible() const
{
  return false;
}







defineType(simpleDyMFoamNumerics);
//addToOpenFOAMCaseElementFactoryTable(simpleDyMFoamNumerics);


simpleDyMFoamNumerics::simpleDyMFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: steadyIncompressibleNumerics(c, ps),
  p_(ps)
{}

 
void simpleDyMFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  steadyIncompressibleNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "simpleDyMFoam");
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["writeInterval"]=OFDictData::data( p_.FEMinterval );
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["startTime"]=OFDictData::data( 0.0 );
  SIMPLE["timeInterval"]=OFDictData::data( p_.FEMinterval );
  
}







//defineType(pimpleDyMFoamNumerics);
//addToOpenFOAMCaseElementFactoryTable(pimpleDyMFoamNumerics);


//pimpleDyMFoamNumerics::pimpleDyMFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
//: pimpleFoamNumerics(c, ps),
//  p_(ps)
//{}

 
//void pimpleDyMFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
//{
//  pimpleFoamNumerics::addIntoDictionaries(dictionaries);
  
//  // ============ setup controlDict ================================
  
//  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
//  controlDict["application"]="pimpleDyMFoam";
  
//}

//ParameterSet pimpleDyMFoamNumerics::defaultParameters()
//{
//    return Parameters::makeDefault();
//}



defineType(cavitatingFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(cavitatingFoamNumerics);


cavitatingFoamNumerics::cavitatingFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 		FieldValue({p_.pamb}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0,0.0,0.0}), volField ) );
  OFcase().addField("rho", FieldInfo(scalarField, 	dimDensity, 		FieldValue({p_.rhoamb}), volField ) );
}

 
void cavitatingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName;
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
    solvers["pcorr"+f.first]=OFcase().stdSymmSolverSetup(1e-7, 0.01*f.second);
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
   OFDictData::dict asd=stdMULESSolverSetup(p_.cAlpha, p_.icAlpha);
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
  div["div(phi,alpha)"]		= "Gauss vanLeer";
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




OFDictData::dict stdMULESSolverSetup(double cAlpha, double icAlpha, double tol, double reltol, bool LTS)
{
  OFDictData::dict d;
  
  d["nAlphaCorr"]=3;
  d["nAlphaSubCycles"]=1;
  d["cAlpha"]=cAlpha;
  d["icAlpha"]=icAlpha;

  d["MULESCorr"]=true;
  d["nLimiterIter"]=15;
  d["alphaApplyPrevCorr"]=LTS;

  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;
  d["maxIter"]=100;

  return d;
}




defineType(LTSInterFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(LTSInterFoamNumerics);


LTSInterFoamNumerics::LTSInterFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: interFoamNumerics(c, ps)
{
}


void LTSInterFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (OFversion()<300)      
  {
    controlDict["application"]="LTSInterFoam";
  }
  
  //double maxCo=10.0, maxAlphaCo=5.0;
  double maxCo=10.0, maxAlphaCo=2.0;
  bool momentumPredictor=false;

  controlDict["maxAlphaCo"]=maxAlphaCo;
  controlDict["maxCo"]=maxCo;

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  if (OFversion()>=230)
    solvers["\"alpha.*\""]=stdMULESSolverSetup(0.25, 0.1, 1e-8, 0.0, true);


  std::string solutionScheme("PIMPLE");
  OFDictData::dict& SOL=fvSolution.subDict(solutionScheme);
  SOL["momentumPredictor"]=momentumPredictor;
  SOL["nCorrectors"]=2;
  SOL["nNonOrthogonalCorrectors"]=1;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=1;
  SOL["cAlpha"]=p_.cAlpha;
  SOL["maxAlphaCo"]=maxAlphaCo;
  SOL["maxCo"]=maxCo;
  SOL["rDeltaTSmoothingCoeff"]=0.05;
  SOL["rDeltaTDampingCoeff"]=0.5;
  SOL["nAlphaSweepIter"]=0;
  SOL["nAlphaSpreadIter"]=0;
  SOL["maxDeltaT"]=1;

  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (OFversion()<300)
  {
    ddt["default"]="localEuler rDeltaT";
  }
  else
  {
    ddt["default"]="localEuler";
  }
  
//   OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
}





defineType(interPhaseChangeFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(interPhaseChangeFoamNumerics);


interPhaseChangeFoamNumerics::interPhaseChangeFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: interFoamNumerics(c, ps),
  p_(ps)
{
}

 
void interPhaseChangeFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName;

  // ============ setup controlDict ================================

//  controlDict["maxDeltaT"]=1.0;

//  controlDict["maxCo"]=0.4;
//  controlDict["maxAlphaCo"]=0.2;
//  if (p_.implicitPressureCorrection)
//  {
//    controlDict["maxCo"]=5;
//    controlDict["maxAlphaCo"]=2.5;
//  }

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  OFDictData::dict alphasol = stdMULESSolverSetup(p_.cAlpha, p_.icAlpha, 1e-10, 0.0, false);
  solvers["\"alpha.*\""]=alphasol;

}





defineType(reactingFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingFoamNumerics);


void reactingFoamNumerics::init()
{
  
  if (OFversion() < 230)
    throw insight::Exception("reactingFoamNumerics currently supports only OF >=230");
  
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({1e5}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		FieldValue({0.0,0.0,0.0}), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature,		FieldValue({300.0}), volField ) );
}


reactingFoamNumerics::reactingFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
    init();
}


void reactingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);
    
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="reactingFoam";

  CompressiblePIMPLESettings ps(p_.time_integration);
  ps.addIntoDictionaries(OFcase(), dictionaries);
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["\"rho.*\""]=OFcase().stdSymmSolverSetup(0, 0);
  solvers["p"]=OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["h"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().stdAsymmSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["pFinal"]=OFcase().stdSymmSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["hFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=OFcase().stdAsymmSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=OFcase().stdAsymmSolverSetup(1e-8, 0);

  solvers["Yi"]=OFcase().stdAsymmSolverSetup(1e-8, 0);
  
  // ============ setup fvSchemes ================================
  
  // check if LES required
  bool LES=p_.forceLES;
  try 
  {
    const turbulenceModel* tm=this->OFcase().get<turbulenceModel>("turbulenceModel");
    LES=LES || (tm->minAccuracyRequirement() >= turbulenceModel::AC_LES);
  }
  catch (...)
  {
    cout<<"Warning: unhandled exception during LES check!"<<endl;
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
    div["div(phi,k)"]="Gauss limitedLinear 1";
  }
  else
  {
    div["div(phi,U)"]="Gauss linearUpwindV limitedGrad";
    div["div(phid,p)"]="Gauss limitedLinear 1";
    div["div(phi,k)"]="Gauss limitedLinear 1";
    div["div(phi,Yi_h)"]="Gauss limitedLinear 1";
    div["div(phi,epsilon)"]="Gauss limitedLinear 1";
    div["div(phi,omega)"]="Gauss limitedLinear 1";
    div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
  }

  div["div((muEff*dev2(T(grad(U)))))"]="Gauss linear";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["interpolate(U)"]="pointLinear";
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}


bool reactingFoamNumerics::isCompressible() const
{
  return true;
}





defineType(reactingParcelFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(reactingParcelFoamNumerics);


reactingParcelFoamNumerics::reactingParcelFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: reactingFoamNumerics(c, ps)
{
}


void reactingParcelFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  reactingFoamNumerics::addIntoDictionaries(dictionaries);
}








defineType(buoyantSimpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(buoyantSimpleFoamNumerics);


void buoyantSimpleFoamNumerics::init()
{

  if (OFversion() < 230)
    throw insight::Exception("buoyantSimpleFoamNumerics currently supports only OF >=230");

  OFcase().addField("p_rgh", FieldInfo(scalarField, 	dimPressure,            FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure,            FieldValue({p_.pinternal}), volField ) );
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
  controlDict["application"]="buoyantSimpleFoam";

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
  solvers["p_rgh"]=OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["k"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
  solvers["h"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
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
    relax["h"]=0.2;
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
    eqnRelax["h"]=0.2;
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
  div["div(phi,K)"]	=	pref+"Gauss linearUpwind limitedGrad";
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


bool buoyantSimpleFoamNumerics::isCompressible() const
{
  return true;
}







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







FSIDisplacementExtrapolationNumerics::FSIDisplacementExtrapolationNumerics(OpenFOAMCase& c, const ParameterSet& p)
: FaNumerics(c), p_(p)
{
  //c.addField("displacement", FieldInfo(vectorField, 	dimLength, 	list_of(0.0)(0.0)(0.0), volField ) );
}
 
void FSIDisplacementExtrapolationNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FaNumerics::addIntoDictionaries(dictionaries);
    
  // ============ setup faSolution ================================
  
  OFDictData::dict& faSolution=dictionaries.lookupDict("system/faSolution");
  
  OFDictData::dict& solvers=faSolution.subDict("solvers");
  solvers["displacement"]=OFcase().stdSymmSolverSetup(1e-7, 0.01);


  // ============ setup faSchemes ================================
  
  OFDictData::dict& faSchemes=dictionaries.lookupDict("system/faSchemes");
  
  OFDictData::dict& grad=faSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  
  OFDictData::dict& div=faSchemes.subDict("divSchemes");
  div["default"]="Gauss linear";

  OFDictData::dict& laplacian=faSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=faSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=faSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

}




defineType(magneticFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(magneticFoamNumerics);

void magneticFoamNumerics::init()
{
  OFcase().addField("psi", FieldInfo(scalarField, 	dimCurrent, 	FieldValue({0.0}), volField ) );
}

magneticFoamNumerics::magneticFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, ""),
  p_(ps)
{
  init();
}


void magneticFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName;
  
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
//   solvers["psi"]=stdSymmSolverSetup(1e-7, 0.01);
  solvers["psi"]=OFcase().GAMGSolverSetup(1e-7, 0.01);
  solvers.subDict("psi")["maxIter"]=5000;
//   solvers["psi"]=GAMGPCGSolverSetup(1e-9, 0.001);

//   OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
//   if (OFversion()<210)
//   {
//     relax["p"]=0.3;
//     relax["U"]=0.7;
//     relax["k"]=0.7;
//     relax["omega"]=0.7;
//     relax["epsilon"]=0.7;
//     relax["nuTilda"]=0.7;
//   }
//   else
//   {
//     OFDictData::dict fieldRelax, eqnRelax;
//     fieldRelax["p"]=0.3;
//     eqnRelax["U"]=0.7;
//     eqnRelax["k"]=0.7;
//     eqnRelax["omega"]=0.7;
//     eqnRelax["epsilon"]=0.7;
//     eqnRelax["nuTilda"]=0.7;
//     relax["fields"]=fieldRelax;
//     relax["equations"]=eqnRelax;
//   }

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 10 );
//   SIMPLE["pRefCell"]=0;
//   SIMPLE["pRefValue"]=0.0;
  
//   if (OFversion()>=210)
//   {
//     OFDictData::dict resCtrl;
//     resCtrl["p"]=1e-4;
//     resCtrl["U"]=1e-3;
//     resCtrl["\"(k|epsilon|omega|nuTilda|R)\""]=1e-4;
//     SIMPLE["residualControl"]=resCtrl;
//   }
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  
  grad["default"]="Gauss linear";
    
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="none";
      
  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
//   laplacian["default"]="Gauss linear localLimited UBlendingFactor 0.66";
  laplacian["default"]="Gauss linear corrected";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
//   snGrad["default"]="localLimited UBlendingFactor 0.66";
  snGrad["default"]="corrected";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
//   fluxRequired["T"]="";

  OFDictData::dict& transportProperties=dictionaries.lookupDict("constant/transportProperties");
  transportProperties.getList("magnets");

}


bool magneticFoamNumerics::isCompressible() const
{
  return false;
}



}

