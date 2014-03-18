/*
    <one line to give the library's name and an idea of what it does.>
    Copyright (C) 2013  hannes <email>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/


#include "basiccaseelements.h"
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


FVNumerics::FVNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "FVNumerics")
{
}

void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=1000.0;
  controlDict["writeControl"]="timeStep";
  controlDict["writeInterval"]=100;
  controlDict["purgeWrite"]=10;
  controlDict["writeFormat"]="binary";
  controlDict["writePrecision"]=6;
  controlDict["writeCompression"]="compressed";
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.addListIfNonexistent("libs");
  controlDict.addSubDictIfNonexistent("functions");
  
  OFDictData::dict& fvSolution=dictionaries.addDictionaryIfNonexistent("system/fvSolution");
  fvSolution.addSubDictIfNonexistent("solvers");
  fvSolution.addSubDictIfNonexistent("relaxationFactors");
  
  OFDictData::dict& fvSchemes=dictionaries.addDictionaryIfNonexistent("system/fvSchemes");
  fvSchemes.addSubDictIfNonexistent("ddtSchemes");
  fvSchemes.addSubDictIfNonexistent("gradSchemes");
  fvSchemes.addSubDictIfNonexistent("divSchemes");
  fvSchemes.addSubDictIfNonexistent("laplacianSchemes");
  fvSchemes.addSubDictIfNonexistent("interpolationSchemes");
  fvSchemes.addSubDictIfNonexistent("snGradSchemes");
  fvSchemes.addSubDictIfNonexistent("fluxRequired");
}

FaNumerics::FaNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "FaNumerics")
{
}

void FaNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& faSolution=dictionaries.addDictionaryIfNonexistent("system/faSolution");
  faSolution.addSubDictIfNonexistent("solvers");
  faSolution.addSubDictIfNonexistent("relaxationFactors");
  
  OFDictData::dict& faSchemes=dictionaries.addDictionaryIfNonexistent("system/faSchemes");
  faSchemes.addSubDictIfNonexistent("ddtSchemes");
  faSchemes.addSubDictIfNonexistent("gradSchemes");
  faSchemes.addSubDictIfNonexistent("divSchemes");
  faSchemes.addSubDictIfNonexistent("laplacianSchemes");
  faSchemes.addSubDictIfNonexistent("interpolationSchemes");
  faSchemes.addSubDictIfNonexistent("snGradSchemes");
  faSchemes.addSubDictIfNonexistent("fluxRequired");
}

tetFemNumerics::tetFemNumerics(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "tetFemNumerics")
{
}

void tetFemNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& tetFemSolution=dictionaries.addDictionaryIfNonexistent("system/tetFemSolution");
  tetFemSolution.addSubDictIfNonexistent("solvers");
}

OFDictData::dict stdAsymmSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="PBiCG";
  d["preconditioner"]="DILU";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  return d;
}

OFDictData::dict stdSymmSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["preconditioner"]="DIC";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  return d;
}

OFDictData::dict GAMGSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="GAMG";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["smoother"]="DICGaussSeidel";
  d["nPreSweeps"]=0;
  d["nPostSweeps"]=2;
  d["cacheAgglomeration"]="on";
  d["agglomerator"]="faceAreaPair";
  d["nCellsInCoarsestLevel"]=10;
  d["mergeLevels"]=1;
  return d;
}

OFDictData::dict GAMGPCGSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="PCG";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  OFDictData::dict pd;
  pd["preconditioner"]="GAMG";
  pd["smoother"]="DICGaussSeidel";
  pd["nPreSweeps"]=0;
  pd["nPostSweeps"]=2;
  pd["cacheAgglomeration"]="on";
  pd["agglomerator"]="faceAreaPair";
  pd["nCellsInCoarsestLevel"]=10;
  pd["mergeLevels"]=1;
  d["preconditioner"]=pd;
  return d;
}

OFDictData::dict smoothSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  d["solver"]="smoothSolver";
  d["smoother"]="GaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["nSweeps"]=1;
  return d;
}


MeshingNumerics::MeshingNumerics(OpenFOAMCase& c)
: FVNumerics(c)
{
}
 
void MeshingNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["writeFormat"]="ascii";
  controlDict["writeCompression"]="uncompressed";
  controlDict["application"]="none";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1.0;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  /*
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-8, 0.1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["p"]=0.3;
    relax["U"]=0.7;
    relax["k"]=0.7;
    relax["omega"]=0.7;
    relax["epsilon"]=0.7;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p"]=0.3;
    eqnRelax["U"]=0.7;
    eqnRelax["k"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 0 );
*/  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="Gauss upwind";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  
  OFDictData::dict& decomposeParDict=dictionaries.addDictionaryIfNonexistent("system/decomposeParDict");
  decomposeParDict["numberOfSubdomains"]=1;
  decomposeParDict["method"]="hierarchical";
  OFDictData::dict coeffs;
  coeffs["n"]=OFDictData::vector3(1,1,1);
  coeffs["delta"]=0.001;
  coeffs["order"]="xyz";
  decomposeParDict["hierarchicalCoeffs"]=coeffs;
  
}

simpleFoamNumerics::simpleFoamNumerics(OpenFOAMCase& c)
: FVNumerics(c)
{
  c.addField("p", FieldInfo(scalarField, 	dimKinPressure, 	list_of(0.0), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
}
 
void simpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="simpleFoam";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1.0;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-8, 0.1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["p"]=0.3;
    relax["U"]=0.7;
    relax["k"]=0.7;
    relax["omega"]=0.7;
    relax["epsilon"]=0.7;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p"]=0.3;
    eqnRelax["U"]=0.7;
    eqnRelax["k"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 0 );
  SIMPLE["pRefCell"]=0;
  SIMPLE["pRefValue"]=0.0;
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";
  if (OFversion()<=160) suf=" cellLimited leastSquares 1"; else suf=" grad(U)";
  div["default"]=pref+"Gauss upwind";
  div["div(phi,U)"]=pref+"Gauss linearUpwindV"+suf;
  if (OFversion()>=210)
  {
    div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  }
  else 
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

pimpleFoamNumerics::pimpleFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c),
  p_(p)
{
  c.addField("p", FieldInfo(scalarField, 	dimKinPressure, 	list_of(0.0), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
}
 
void pimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="pimpleFoam";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1.0;
  controlDict["adjustTimeStep"]=p_.adjustTimeStep();
  controlDict["maxCo"]=p_.maxCo();
  controlDict["maxDeltaT"]=p_.maxDeltaT();
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGPCGSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["pFinal"]=GAMGPCGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["epsilonFinal"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  PIMPLE["nCorrectors"]=2;
  PIMPLE["nOuterCorrectors"]=1;
  PIMPLE["nNonOrthogonalCorrectors"]=0;
  PIMPLE["pRefCell"]=0;
  PIMPLE["pRefValue"]=0.0;
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss limitedLinear 1";
  div["div(phi,U)"]="Gauss limitedLinearV 1";
  if (OFversion()>=210)
  {
    div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  }
  else 
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}


simpleDyMFoamNumerics::simpleDyMFoamNumerics(OpenFOAMCase& c, const Parameters& p)
: simpleFoamNumerics(c),
  p_(p)
{}
 
void simpleDyMFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  simpleFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="simpleDyMFoam";
  controlDict["writeInterval"]=OFDictData::data( p_.FEMinterval() );
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["startTime"]=OFDictData::data( 0.0 );
  SIMPLE["timeInterval"]=OFDictData::data( p_.FEMinterval() );
  
}


cavitatingFoamNumerics::cavitatingFoamNumerics(OpenFOAMCase& c)
: FVNumerics(c)
{
  c.addField("p", FieldInfo(scalarField, 	dimPressure, 		list_of(1e5), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
  c.addField("rho", FieldInfo(scalarField, 	dimDensity, 		list_of(0.0), volField ) );
}
 
void cavitatingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="cavitatingFoam";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1e-6;
  controlDict["adjustTimeStep"]=true;
  controlDict["maxCo"]=0.5;
  controlDict["maxAcousticCo"]=50.;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=stdSymmSolverSetup(1e-7, 0.0);
  solvers["pFinal"]=stdSymmSolverSetup(1e-7, 0.0);
  solvers["rho"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["U"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["k"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omega"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("PISO");
  SIMPLE["nCorrectors"]=OFDictData::data( 2 );
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 0 );
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
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

interFoamNumerics::interFoamNumerics(OpenFOAMCase& c)
: FVNumerics(c)
{
  if (OFversion()<=160)
    pname_="pd";
  else
    pname_="p_rgh";
  
  c.addField(pname_, FieldInfo(scalarField, 	dimPressure, 		list_of(0.0), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
  c.addField("alpha1", FieldInfo(scalarField, 	dimless, 		list_of(0.0), volField ) );
}
 
void interFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="interFoam";
  controlDict["endTime"]=1000.0;
  controlDict["deltaT"]=1e-6;

  controlDict["adjustTimeStep"]=true;
  controlDict["maxCo"]=0.5;
  controlDict["maxAlphaCo"]=50.;
  controlDict["maxDeltaT"]=1.0;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["pcorr"]=GAMGPCGSolverSetup(1e-6, 0.0);
  solvers[pname_]=GAMGPCGSolverSetup(1e-7, 0.01);
  solvers[pname_+"Final"]=GAMGPCGSolverSetup(1e-7, 0.0);
  
  solvers["U"]=smoothSolverSetup(1e-8, 0);
  solvers["k"]=smoothSolverSetup(1e-8, 0);
  solvers["omega"]=smoothSolverSetup(1e-8, 0);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0);
  
  solvers["UFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["kFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["omegaFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["epsilonFinal"]=smoothSolverSetup(1e-10, 0);

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["U"]=0.7;
  }
  else
  {
    OFDictData::dict /*fieldRelax,*/ eqnRelax;
    eqnRelax["U"]=0.7;
//     relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  std::string solutionScheme("PISO");
  if (OFversion()>=210) solutionScheme="PIMPLE";
  OFDictData::dict& SOL=fvSolution.addSubDictIfNonexistent(solutionScheme);
  SOL["momentumPredictor"]=true;
  SOL["nCorrectors"]=2;
  SOL["nNonOrthogonalCorrectors"]=1;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=4;
  SOL["cAlpha"]=1.0;
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  if (OFversion()==160) 
    suf="cellLimited leastSquares 1";
  else 
    suf="grad(U)";
  div["div(rho*phi,U)"]="Gauss linearUpwind "+suf;
  div["div(rhoPhi,U)"]="Gauss linearUpwind "+suf; // for interPhaseChangeFoam
  div["div(phi,alpha)"]="Gauss vanLeer";
  div["div(phirb,alpha)"]="Gauss interfaceCompression";
  div["div(phi,k)"]="Gauss upwind";
  div["div(phi,epsilon)"]="Gauss upwind";
  div["div(phi,R)"]="Gauss upwind";
  div["div(R)"]="Gauss linear";
  div["div(phi,nuTilda)"]="Gauss upwind";
  if (OFversion()>=210)
    div["div((muEff*dev(T(grad(U)))))"]="Gauss linear";
  else
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear limited 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired[pname_]="";
  fluxRequired["pcorr"]="";
  fluxRequired["alpha"]="";
  fluxRequired["alpha1"]="";
}

interPhaseChangeFoamNumerics::interPhaseChangeFoamNumerics(OpenFOAMCase& c)
: interFoamNumerics(c)
{
}
 
void interPhaseChangeFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  OFDictData::dict alphasol = stdAsymmSolverSetup(1e-10, 0.0);
  alphasol["maxUnboundedness"]=1e-5;
  alphasol["CoCoeff"]=0;
  alphasol["maxIter"]=5;
  alphasol["nLimiterIter"]=2;

  solvers["alpha1"]=alphasol;

  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  if (OFversion()==160) 
    suf="cellLimited leastSquares 1";
  else 
    suf="grad(U)";
  div["div(rhoPhi,U)"]="Gauss linearUpwind "+suf; // for interPhaseChangeFoam

}

FSIDisplacementExtrapolationNumerics::FSIDisplacementExtrapolationNumerics(OpenFOAMCase& c)
: FaNumerics(c)
{
  //c.addField("displacement", FieldInfo(vectorField, 	dimLength, 	list_of(0.0)(0.0)(0.0), volField ) );
}
 
void FSIDisplacementExtrapolationNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FaNumerics::addIntoDictionaries(dictionaries);
    
  // ============ setup faSolution ================================
  
  OFDictData::dict& faSolution=dictionaries.lookupDict("system/faSolution");
  
  OFDictData::dict& solvers=faSolution.subDict("solvers");
  solvers["displacement"]=stdSymmSolverSetup(1e-7, 0.01);


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

thermodynamicModel::thermodynamicModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "thermodynamicModel")
{
}

cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, const Parameters& p)
: thermodynamicModel(c),
  p_(p)
{
}

void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.addDictionaryIfNonexistent("constant/thermodynamicProperties");
  thermodynamicProperties["barotropicCompressibilityModel"]="linear";
  thermodynamicProperties["psiv"]=OFDictData::dimensionedData("psiv", 
							      OFDictData::dimension(0, -2, 2), 
							      p_.psiv());
  thermodynamicProperties["rholSat"]=OFDictData::dimensionedData("rholSat", 
								 OFDictData::dimension(1, -3), 
								 p_.rholSat());
  thermodynamicProperties["psil"]=OFDictData::dimensionedData("psil", 
								 OFDictData::dimension(0, -2, 2), 
								 p_.psil());
  thermodynamicProperties["pSat"]=OFDictData::dimensionedData("pSat", 
								 OFDictData::dimension(1, -1, -2), 
								 p_.pSat());
  thermodynamicProperties["rhoMin"]=OFDictData::dimensionedData("rhoMin", 
								 OFDictData::dimension(1, -3), 
								 p_.rhoMin());
}

gravity::gravity(OpenFOAMCase& c, Parameters const& p)
: OpenFOAMCaseElement(c, "gravity"),
  p_(p)
{
}

void gravity::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& g=dictionaries.addDictionaryIfNonexistent("constant/g");
  g["dimensions"]="[0 1 -2 0 0 0 0]";
  OFDictData::list gv;
  for (int i=0; i<3; i++) gv.push_back(p_.g()(i));
  g["value"]=gv;
}
  
transportModel::transportModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "transportModel")
{
}

MRFZone::MRFZone(OpenFOAMCase& c, Parameters const& p )
: OpenFOAMCaseElement(c, "MRFZone"+p.name()),
  p_(p)
{
}

void MRFZone::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()<220)
  {
    throw insight::Exception("Not yet supported!");
  }
  else
  {
    OFDictData::dict coeffs;
    OFDictData::list nrp; nrp.resize(p_.nonRotatingPatches().size());
    copy(p_.nonRotatingPatches().begin(), p_.nonRotatingPatches().end(), nrp.begin());
    
    coeffs["nonRotatingPatches"]=nrp;
    coeffs["origin"]=OFDictData::vector3(p_.rotationCentre());
    coeffs["axis"]=OFDictData::vector3(p_.rotationAxis());
    coeffs["omega"]=2.*M_PI*p_.rpm()/60.;

    OFDictData::dict fod;
    fod["type"]="MRFSource";
    fod["active"]=true;
    fod["selectionMode"]="cellZone";
    fod["cellZone"]=p_.name();
    fod["MRFSourceCoeffs"]=coeffs;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[p_.name()]=fod;     
  }
}

PressureGradientSource::PressureGradientSource(OpenFOAMCase& c, Parameters const& p )
: OpenFOAMCaseElement(c, "PressureGradientSource"),
  p_(p)
{
}

void PressureGradientSource::addIntoDictionaries(OFdicts& dictionaries) const
{
  if (OFversion()>=220)
  {
    OFDictData::dict coeffs;    
    OFDictData::list flds; flds.push_back("U");
    coeffs["fieldNames"]=flds;
    coeffs["Ubar"]=OFDictData::vector3(p_.Ubar());

    OFDictData::dict fod;
    fod["type"]="pressureGradientExplicitSource";
    fod["active"]=true;
    fod["selectionMode"]="all";
    fod["pressureGradientExplicitSourceCoeffs"]=coeffs;
    
    OFDictData::dict& fvOptions=dictionaries.addDictionaryIfNonexistent("system/fvOptions");
    fvOptions[name()]=fod;  
  }
  else
  {
    // for channelFoam:
    OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
    transportProperties["Ubar"]=OFDictData::dimensionedData("Ubar", dimVelocity, OFDictData::vector3(p_.Ubar()));
  }
}

singlePhaseTransportProperties::singlePhaseTransportProperties(OpenFOAMCase& c, Parameters const& p )
: transportModel(c),
  p_(p)
{
}
 
void singlePhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["transportModel"]="Newtonian";
  transportProperties["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu());
}

twoPhaseTransportProperties::twoPhaseTransportProperties(OpenFOAMCase& c, Parameters const& p )
: transportModel(c),
  p_(p)
{
}
 
void twoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  
  OFDictData::dict& twoPhase=transportProperties.addSubDictIfNonexistent("twoPhase");
  twoPhase["transportModel"]="twoPhase";
  twoPhase["phase1"]="phase1";
  twoPhase["phase2"]="phase2";
  
  OFDictData::dict& phase1=transportProperties.addSubDictIfNonexistent("phase1");
  phase1["transportModel"]="Newtonian";
  phase1["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu1());
  phase1["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho1());
  
  OFDictData::dict& phase2=transportProperties.addSubDictIfNonexistent("phase2");
  phase2["transportModel"]="Newtonian";
  phase2["nu"]=OFDictData::dimensionedData("nu", OFDictData::dimension(0, 2, -1), p_.nu2());
  phase2["rho"]=OFDictData::dimensionedData("rho", OFDictData::dimension(1, -3), p_.rho2());

  transportProperties["sigma"]=OFDictData::dimensionedData("sigma", OFDictData::dimension(1, 0, -2), p_.sigma());

}

namespace phaseChangeModels
{

SchnerrSauer::SchnerrSauer(Parameters const& p)
: p_(p)
{
}

void SchnerrSauer::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["phaseChangeTwoPhaseMixture"]="SchnerrSauer";
  
  OFDictData::dict& coeffs=transportProperties.addSubDictIfNonexistent("SchnerrSauerCoeffs");
  coeffs["n"] = OFDictData::dimensionedData("n", OFDictData::dimension(0, -3), p_.n());
  coeffs["dNuc"] = OFDictData::dimensionedData("dNuc", OFDictData::dimension(0, 1), p_.dNuc());
  coeffs["Cc"] = OFDictData::dimensionedData("Cc", OFDictData::dimension(), p_.Cc());
  coeffs["Cv"] = OFDictData::dimensionedData("Cv", OFDictData::dimension(), p_.Cv());
}

}

cavitationTwoPhaseTransportProperties::cavitationTwoPhaseTransportProperties
(
  OpenFOAMCase& c, 
  Parameters const& p
)
: twoPhaseTransportProperties(c, p),
  p_(p)
{
}

void cavitationTwoPhaseTransportProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  twoPhaseTransportProperties::addIntoDictionaries(dictionaries);
  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties["pSat"]=OFDictData::dimensionedData("pSat", OFDictData::dimension(1, -1, -2), p_.psat());
  p_.model()->addIntoDictionaries(dictionaries);
}
  
  
dynamicMesh::dynamicMesh(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "dynamicMesh")
{
}

velocityTetFEMMotionSolver::velocityTetFEMMotionSolver(OpenFOAMCase& c)
: dynamicMesh(c),
  tetFemNumerics_(c)
{
  c.addField("motionU", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), tetField ) );
}

void velocityTetFEMMotionSolver::addIntoDictionaries(OFdicts& dictionaries) const
{
  tetFemNumerics_.addIntoDictionaries(dictionaries);

  OFDictData::dict& tetFemSolution=dictionaries.addDictionaryIfNonexistent("system/tetFemSolution");
  OFDictData::dict& solvers = tetFemSolution.subDict("solvers");
  solvers["motionU"]=stdSymmSolverSetup();
  
  OFDictData::dict& dynamicMeshDict=dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");
  dynamicMeshDict["dynamicFvMesh"]=OFDictData::data("dynamicMotionSolverFvMesh");
  dynamicMeshDict["solver"]=OFDictData::data("laplaceFaceDecomposition");
  if (dynamicMesh::OFversion()<=160)
  {
    dynamicMeshDict["diffusivity"]=OFDictData::data("uniform");
    dynamicMeshDict["frozenDiffusion"]=OFDictData::data(false);
    dynamicMeshDict["twoDMotion"]=OFDictData::data(false);
  }
  else
  {
    throw insight::Exception("No tetFEMMotionsolver available for OF>1.6 ext");
  }
}

displacementFvMotionSolver::displacementFvMotionSolver(OpenFOAMCase& c)
: dynamicMesh(c)
{
}

void displacementFvMotionSolver::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& dynamicMeshDict=dictionaries.addDictionaryIfNonexistent("constant/dynamicMeshDict");
  dynamicMeshDict["dynamicFvMesh"]=OFDictData::data("dynamicMotionSolverFvMesh");
  dynamicMeshDict["solver"]=OFDictData::data("displacementLaplacian");
  if (OFversion()<220)
  {
    dynamicMeshDict["diffusivity"]=OFDictData::data("uniform");
  }
  else
  {
    OFDictData::dict sd;
    sd["diffusivity"]=OFDictData::data("uniform");
    dynamicMeshDict["displacementLaplacianCoeffs"]=sd;
  }
}

outputFilterFunctionObject::outputFilterFunctionObject(OpenFOAMCase& c, Parameters const &p )
: OpenFOAMCaseElement(c, p.name()+"outputFilterFunctionObject"),
  p_(p)
{
}

void outputFilterFunctionObject::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod=functionObjectDict();
  fod["enabled"]=true;
  fod["outputControl"]=p_.outputControl();
  fod["outputInterval"]=p_.outputInterval();
  fod["timeStart"]=p_.timeStart();
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.addSubDictIfNonexistent("functions")[p_.name()]=fod;
}

fieldAveraging::fieldAveraging(OpenFOAMCase& c, Parameters const &p )
: outputFilterFunctionObject(c, p),
  p_(p)
{
}

OFDictData::dict fieldAveraging::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="fieldAverage";
  OFDictData::list libl; libl.push_back("\"libfieldFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  
  OFDictData::list fl;
  BOOST_FOREACH(const std::string& fln, p_.fields())
  {
    fl.push_back(fln);
    OFDictData::dict cod;
    cod["mean"]=true;
    cod["prime2Mean"]=true;
    cod["base"]="time";
    fl.push_back(cod);
  }
  fod["fields"]=fl;
  
  return fod;
}
  
probes::probes(OpenFOAMCase& c, Parameters const &p )
: outputFilterFunctionObject(c, p),
  p_(p)
{
}

OFDictData::dict probes::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="probes";
  OFDictData::list libl; libl.push_back("\"libsampling.so\"");
  fod["functionObjectLibs"]=libl;
  
  OFDictData::list pl;
  BOOST_FOREACH(const arma::mat& lo, p_.probeLocations())
  {
    pl.push_back(OFDictData::vector3(lo));
  }
  fod["probeLocations"]=pl;
  
  OFDictData::list fl; fl.resize(p_.fields().size());
  copy(p_.fields().begin(), p_.fields().end(), fl.begin());
  fod["fields"]=fl;
  
  return fod;
}

twoPointCorrelation::twoPointCorrelation(OpenFOAMCase& c, Parameters const &p )
: outputFilterFunctionObject(c, p),
  p_(p)
{
}

OFDictData::dict twoPointCorrelation::csysConfiguration() const
{
  OFDictData::dict csys;
  csys["type"]="cartesian";
  csys["origin"]=OFDictData::vector3(0,0,0);
  csys["e1"]=OFDictData::vector3(1,0,0);
  csys["e2"]=OFDictData::vector3(0,1,0);
  return csys;
}

OFDictData::dict twoPointCorrelation::functionObjectDict() const
{
  OFDictData::dict fod;
  fod["type"]="twoPointCorrelation";
  OFDictData::list libl; libl.push_back("\"libLESFunctionObjects.so\"");
  fod["functionObjectLibs"]=libl;
  fod["enabled"]=true;
  fod["outputControl"]=p_.outputControl();
  fod["outputInterval"]=p_.outputInterval();
  fod["timeStart"]=p_.timeStart();
  
  fod["p0"]=OFDictData::vector3(p_.p0());
  fod["directionSpan"]=OFDictData::vector3(p_.directionSpan());
  fod["np"]=p_.np();
  fod["homogeneousTranslationUnit"]=OFDictData::vector3(p_.homogeneousTranslationUnit());
  fod["nph"]=p_.nph();

  fod["csys"]=csysConfiguration();
  
  return fod;
}

boost::ptr_vector<arma::mat> twoPointCorrelation::readCorrelations(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& tpcName)
{
  std::vector<double> t;
  std::vector<double> profs[6];
  int np=-1;
  
  path fp;
  if (c.OFversion()<=160)
    fp=absolute(location)/tpcName;
  else
    fp=absolute(location)/"postProcessing"/tpcName;
  
  TimeDirectoryList tdl=listTimeDirectories(fp);
  
  BOOST_FOREACH(const TimeDirectoryList::value_type& td, tdl)
  {
    std::ifstream f( (td.second/"twoPointCorrelation.dat").c_str());
    while (!f.eof())
    {
      string line;
      getline(f, line);
      if (f.fail()) break;
      cout<<line<<endl;
      if (!starts_with(line, "#"))
      {
// 	erase_all(line, "(");
// 	erase_all(line, ")");
// 	replace_all(line, ",", " ");
// 	replace_all(line, "  ", " ");
	
	// split into component tpcs
	std::vector<string> strs;
	boost::split(strs, line, boost::is_any_of("\t"));
	
	t.push_back(lexical_cast<double>(strs[0]));
	
	if (strs.size()!=7)
	  throw insight::Exception("Expected profiles for 6 tensor components in twoPointCorrelation results!");
	
	for (int k=1; k<=6; k++)
	{
	  std::vector<string> pts;
	  boost::split(pts, strs[k], boost::is_any_of(" "));
	  if (np<0) 
	    np=pts.size();
	  else if (np!=pts.size())
	    throw insight::Exception("Expected uniform number of sampling point in twoPointCorrelation results!");
	  
	  BOOST_FOREACH(const std::string& s, pts)
	  {
	    profs[k-1].push_back(lexical_cast<double>(s));
	  }
	}
      }
    }
  }
  
  ptr_vector<arma::mat> res;
  res.push_back(new arma::mat(t.data(), t.size(), 1));
  for (int k=0; k<6; k++) res.push_back(new arma::mat(profs[k].data(), t.size(), np));
  
  return res;  
}

cylindricalTwoPointCorrelation::cylindricalTwoPointCorrelation(OpenFOAMCase& c, Parameters const &p )
: twoPointCorrelation(c, p),
  p_(p)
{
}

OFDictData::dict cylindricalTwoPointCorrelation::csysConfiguration() const
{
  OFDictData::dict csys;
  csys["type"]="cylindrical";
  csys["origin"]=OFDictData::vector3(0,0,0);
  csys["e3"]=OFDictData::vector3(p_.ez());
  csys["e1"]=OFDictData::vector3(p_.er());
  csys["degrees"]=p_.degrees();
  return csys;
}

forces::forces(OpenFOAMCase& c, Parameters const &p )
: OpenFOAMCaseElement(c, p.name()+"forces"),
  p_(p)
{
}

void forces::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict fod;
  fod["type"]="forces";
  OFDictData::list libl; libl.push_back("\"libforces.so\"");
  fod["functionObjectLibs"]=libl;
  fod["log"]=true;
  fod["outputControl"]=p_.outputControl();
  fod["outputInterval"]=p_.outputInterval();
  
  OFDictData::list pl;
  BOOST_FOREACH(const std::string& lo, p_.patches())
  {
    pl.push_back(lo);
  }
  fod["patches"]=pl;
  fod["pName"]=p_.pName();
  fod["UName"]=p_.UName();
  fod["rhoName"]=p_.rhoName();
  fod["rhoInf"]=p_.rhoInf();
  
  fod["CofR"]=OFDictData::vector3(p_.CofR());
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict.addSubDictIfNonexistent("functions")[p_.name()]=fod;
}

arma::mat forces::readForces(const OpenFOAMCase& c, const boost::filesystem::path& location, const std::string& foName)
{
  arma::mat fl;
  
  path fp;
  if (c.OFversion()<=160)
    fp=absolute(location)/foName;
  else
    fp=absolute(location)/"postProcessing"/foName;
  
  TimeDirectoryList tdl=listTimeDirectories(fp);
  
  BOOST_FOREACH(const TimeDirectoryList::value_type& td, tdl)
  {
    std::ifstream f( (td.second/"forces.dat").c_str());
    while (!f.eof())
    {
      string line;
      getline(f, line);
      if (f.fail()) break;
      cout<<line<<endl;
      if (!starts_with(line, "#"))
      {
	erase_all(line, "(");
	erase_all(line, ")");
	replace_all(line, ",", " ");
	replace_all(line, "  ", " ");
	
	std::vector<string> strs;
	boost::split(strs, line, boost::is_any_of(" \t"));
	
	if (fl.n_rows==0) 
	  fl.set_size(1, strs.size());
	else
	  fl.resize(fl.n_rows+1, fl.n_cols);
	int j=fl.n_rows-1;
	
	int k=0;
	BOOST_FOREACH(const string& e, strs)
	{
	  fl(j,k++)=lexical_cast<double>(e);
	}
      }
    }
  }
  
  if (c.OFversion()>=220)
  {
    // remove porous forces
    fl.shed_cols(16,18);
    fl.shed_cols(7,9);
  }
  
  return fl;
}

  
defineType(turbulenceModel);
defineFactoryTable(turbulenceModel, turbulenceModel::ConstrP);

turbulenceModel::turbulenceModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "turbulenceModel")
{
}

turbulenceModel::turbulenceModel(const turbulenceModel::ConstrP& c)
: OpenFOAMCaseElement(c.get<0>(), "turbulenceModel")
{
}


defineType(RASModel);

RASModel::RASModel(OpenFOAMCase& c)
: turbulenceModel(c)
{
}

RASModel::RASModel(const turbulenceModel::ConstrP& c)
: turbulenceModel(c)
{
}

void RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.addDictionaryIfNonexistent("constant/turbulenceProperties");
  turbProperties["simulationType"]="RASModel";
}

defineType(LESModel);

LESModel::LESModel(OpenFOAMCase& c)
: turbulenceModel(c)
{
}

LESModel::LESModel(const turbulenceModel::ConstrP& c)
: turbulenceModel(c)
{
}

void LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& turbProperties=dictionaries.addDictionaryIfNonexistent("constant/turbulenceProperties");
  turbProperties["simulationType"]="LESModel";
}

defineType(laminar_RASModel);
addToFactoryTable(turbulenceModel, laminar_RASModel, turbulenceModel::ConstrP);

laminar_RASModel::laminar_RASModel(OpenFOAMCase& c)
: RASModel(c)
{}
  
laminar_RASModel::laminar_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{}
  
void laminar_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="laminar";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool laminar_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  return false;
}

defineType(oneEqEddy_LESModel);
addToFactoryTable(turbulenceModel, oneEqEddy_LESModel, turbulenceModel::ConstrP);

void oneEqEddy_LESModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("nuSgs", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}
  

oneEqEddy_LESModel::oneEqEddy_LESModel(OpenFOAMCase& c)
: LESModel(c)
{
  addFields();
}

oneEqEddy_LESModel::oneEqEddy_LESModel(const ConstrP& c)
: LESModel(c)
{
  addFields();
}

void oneEqEddy_LESModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  LESModel::addIntoDictionaries(dictionaries);
  
  OFDictData::dict& LESProperties=dictionaries.addDictionaryIfNonexistent("constant/LESProperties");
  LESProperties["LESModel"]="oneEqEddy";
  LESProperties["delta"]="cubeRootVol";
  LESProperties["printCoeffs"]=true;
  OFDictData::dict crvc;
  crvc["deltaCoeff"]=1.0;
  LESProperties["cubeRootVolCoeffs"]=crvc;
  LESProperties.addSubDictIfNonexistent("laminarCoeffs");
}

bool oneEqEddy_LESModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="fixedValue";
    BC["value"]="uniform 0";
    return true;
  }
  else if (fieldname == "nuSgs")
  {
    BC["type"]="zeroGradient";
    return true;
  }
  
  return false;
}


defineType(kOmegaSST_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_RASModel, turbulenceModel::ConstrP);

void kOmegaSST_RASModel::addFields()
{
  OFcase().addField("k", 	FieldInfo(scalarField, 	dimKinEnergy, 	list_of(1e-10), volField ) );
  OFcase().addField("omega", 	FieldInfo(scalarField, 	OFDictData::dimension(0, 0, -1), 	list_of(1.0), volField ) );
  OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}

kOmegaSST_RASModel::kOmegaSST_RASModel(OpenFOAMCase& c)
: RASModel(c)
{
  addFields();
}
  
kOmegaSST_RASModel::kOmegaSST_RASModel(const turbulenceModel::ConstrP& c)
: RASModel(c)
{
  addFields();
}
  
void kOmegaSST_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSSTCoeffs");
}

bool kOmegaSST_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]=OFDictData::data("kqRWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]=OFDictData::data("omegaWallFunction");
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["beta1"]=0.075;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]=OFDictData::data("nutkWallFunction");
    BC["value"]=OFDictData::data("uniform 1e-10");
    return true;
  }
  
  return false;
}

defineType(kOmegaSST_LowRe_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST_LowRe_RASModel, turbulenceModel::ConstrP);

kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{}
  
kOmegaSST_LowRe_RASModel::kOmegaSST_LowRe_RASModel(const turbulenceModel::ConstrP& c)
: kOmegaSST_RASModel(c)
{}
  
void kOmegaSST_LowRe_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST_LowRe";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSST_LowReCoeffs");
}

bool kOmegaSST_LowRe_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if ( (fieldname == "k") || (fieldname == "omega") )
  {
    BC["type"]=OFDictData::data("zeroGradient");
    return true;
  }
  return false;
}

defineType(kOmegaSST2_RASModel);
addToFactoryTable(turbulenceModel, kOmegaSST2_RASModel, turbulenceModel::ConstrP);

void kOmegaSST2_RASModel::addFields()
{
  OFcase().addField("nut", 	FieldInfo(scalarField, 	dimKinViscosity, 	list_of(1e-10), volField ) );
}

kOmegaSST2_RASModel::kOmegaSST2_RASModel(OpenFOAMCase& c)
: kOmegaSST_RASModel(c)
{
  kOmegaSST2_RASModel::addFields();
}
  
kOmegaSST2_RASModel::kOmegaSST2_RASModel(const turbulenceModel::ConstrP& c)
: kOmegaSST_RASModel(c)
{
  kOmegaSST2_RASModel::addFields();  
}
  
void kOmegaSST2_RASModel::addIntoDictionaries(OFdicts& dictionaries) const
{
  RASModel::addIntoDictionaries(dictionaries);

  OFDictData::dict& RASProperties=dictionaries.addDictionaryIfNonexistent("constant/RASProperties");
  RASProperties["RASModel"]="kOmegaSST2";
  RASProperties["turbulence"]="true";
  RASProperties["printCoeffs"]="true";
  RASProperties.addSubDictIfNonexistent("kOmegaSST2");
  
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libkOmegaSST2.so\"") );
}

bool kOmegaSST2_RASModel::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "k")
  {
    BC["type"]="kqRWallFunction";
    BC["value"]="uniform 1e-10";
    return true;
  }
  else if (fieldname == "omega")
  {
    BC["type"]="hybridOmegaWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["tw"]=0.057;
    BC["value"]="uniform 1";
    return true;
  }
  else if (fieldname == "nut")
  {
    BC["type"]="nutHybridWallFunction2";
    BC["Cmu"]=0.09;
    BC["kappa"]=0.41;
    BC["E"]=9.8;
    BC["value"]="uniform 1e-10";
    return true;
  }
  return false;
}


BoundaryCondition::BoundaryCondition(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"BC"),
  patchName_(patchName),
  type_(boundaryDict.subDict(patchName).getString("type")),
  nFaces_(boundaryDict.subDict(patchName).getInt("nFaces")),
  startFace_(boundaryDict.subDict(patchName).getInt("startFace"))  
{
}

void BoundaryCondition::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["type"]=type_;
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
}

void BoundaryCondition::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
  }
}

void BoundaryCondition::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);
  OFDictData::dict bndsubd;
  addOptionsToBoundaryDict(bndsubd);
  
  insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
}

void BoundaryCondition::insertIntoBoundaryDict
(
  OFdicts& dictionaries, 
  const string& patchName,
  const OFDictData::dict& bndsubd
)
{
  // contents is created as list of string / subdict pairs
  // patches have to appear ordered by "startFace"!
  OFDictData::dict& boundaryDict=dictionaries.addDictionaryIfNonexistent("constant/polyMesh/boundary");
  if (boundaryDict.size()==0)
    boundaryDict.addListIfNonexistent("");
  
  OFDictData::list& bl=
    *boost::get<OFDictData::list>( &boundaryDict.begin()->second );
  
  //std::cout<<"Configuring "<<patchName_<<std::endl;
  // search, if patchname is already present; replace, if yes
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (std::string *name = boost::get<std::string>(&(*i)))
    {
      //cout<<"found "<<*name<<endl;
      if ( *name == patchName )
      {
	i++;
	*i=bndsubd;
	return;
      }
    }
  }
  
  // not found, insert (at the right location)
  OFDictData::list::iterator j = bl.end();
  for(OFDictData::list::iterator i=bl.begin(); i!=bl.end(); i++)
  {
    if (OFDictData::dict *d = boost::get<OFDictData::dict>(&(*i)))
    {
      if (d->getInt("startFace") > bndsubd.getInt("startFace") ) 
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
      // patch with 0 faces has to be inserted before the face with the same start address but nonzero size
      if ( (d->getInt("startFace") == bndsubd.getInt("startFace") ) && (bndsubd.getInt("nFaces") == 0) )
      {
	//std::cout << "Inserting before " << *boost::get<std::string>(&(*(i-1))) << std::endl;
	j=i-1;
	break;
      }
    }
  }
  j = bl.insert( j, OFDictData::data(patchName) );
  bl.insert( j+1, bndsubd );
  
  OFDictData::dict::iterator oe=boundaryDict.begin();
  std::swap( boundaryDict[lexical_cast<std::string>(bl.size()/2)], oe->second );
  boundaryDict.erase(oe);
}

bool BoundaryCondition::providesBCsForPatch(const std::string& patchName) const
{
  return (patchName == patchName_);
}


SimpleBC::SimpleBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, const std::string className)
: BoundaryCondition(c, patchName, boundaryDict),
  className_(className)
{
  type_=className;
}

void SimpleBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (className_=="cyclic") && ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
      BC["type"]=OFDictData::data(className_);
  }
}


CyclicPairBC::CyclicPairBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict)
: OpenFOAMCaseElement(c, patchName+"CyclicBC"),
  patchName_(patchName)
{
  if (c.OFversion()>=210)
  {
    nFaces_=boundaryDict.subDict(patchName_+"_half0").getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_+"_half0").getInt("startFace");
    nFaces1_=boundaryDict.subDict(patchName_+"_half1").getInt("nFaces");
    startFace1_=boundaryDict.subDict(patchName_+"_half1").getInt("startFace");
  }
  else
  {
    nFaces_=boundaryDict.subDict(patchName_).getInt("nFaces");
    startFace_=boundaryDict.subDict(patchName_).getInt("startFace");
  }
}

void CyclicPairBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict bndsubd, bndsubd1;
  bndsubd["type"]="cyclic";
  bndsubd["nFaces"]=nFaces_;
  bndsubd["startFace"]=startFace_;
  bndsubd1["type"]="cyclic";
  bndsubd1["nFaces"]=nFaces1_;
  bndsubd1["startFace"]=startFace1_;
  
  if (OFversion()>=210)
  {
    bndsubd["neighbourPatch"]=patchName_+"_half1";
    bndsubd1["neighbourPatch"]=patchName_+"_half0";
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half0", bndsubd);
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_+"_half1", bndsubd1);
  }
  else
  {
    BoundaryCondition::insertIntoBoundaryDict(dictionaries, patchName_, bndsubd);
  }
}

void CyclicPairBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dictFile& fieldDict=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second);
    OFDictData::dict& boundaryField=fieldDict.addSubDictIfNonexistent("boundaryField");
    
    if (OFversion()>=210)
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_+"_half0");
      OFDictData::dict& BC1=boundaryField.addSubDictIfNonexistent(patchName_+"_half1");
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC1);
      }
      else
      {
	BC["type"]="cyclic";
	BC1["type"]="cyclic";
      }
    }
    else
    {
      OFDictData::dict& BC=boundaryField.addSubDictIfNonexistent(patchName_);
      
      if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      {
	noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
      }
      else
      {
	BC["type"]="cyclic";
      }
    }
  }
}

bool CyclicPairBC::providesBCsForPatch(const std::string& patchName) const
{
  if (OFversion()>=210)
    return ( (patchName == patchName_+"_half0") || (patchName == patchName_+"_half1") );
  else
    return ( patchName == patchName_ );
}

GGIBC::GGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void GGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    //bndDict["transform"]= "rotational";    
  }
  else
  {
    bndDict["type"]="ggi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["zone"]=p_.zone();
  }
}

void GGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("ggi");
    }
  }
}


CyclicGGIBC::CyclicGGIBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, 
	Parameters const &p )
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void CyclicGGIBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  bndDict["nFaces"]=nFaces_;
  bndDict["startFace"]=startFace_;
  if (OFversion()>=210)
  {
    bndDict["type"]="cyclicAMI";
    bndDict["neighbourPatch"]= p_.shadowPatch();
    bndDict["matchTolerance"]= 0.001;
    bndDict["transform"]= "rotational";    
    bndDict["rotationCentre"]=OFDictData::vector3(p_.rotationCentre());
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
  }
  else
  {
    bndDict["type"]="cyclicGgi";
    bndDict["shadowPatch"]= p_.shadowPatch();
    bndDict["separationOffset"]=OFDictData::vector3(p_.separationOffset());
    bndDict["bridgeOverlap"]=p_.bridgeOverlap();
    bndDict["rotationAxis"]=OFDictData::vector3(p_.rotationAxis());
    bndDict["rotationAngle"]=p_.rotationAngle();
    bndDict["zone"]=p_.zone();
  }
}

void CyclicGGIBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    if ( ((field.first=="motionU")||(field.first=="pointDisplacement")) )
      noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC);
    else
    {
      if (OFversion()>=220)
	BC["type"]=OFDictData::data("cyclicAMI");
      else
	BC["type"]=OFDictData::data("cyclicGgi");
    }
  }
}

namespace multiphaseBC
{

uniformPhases::uniformPhases( Parameters const& p )
: p_(p)
{}

bool uniformPhases::addIntoFieldDictionary(const std::string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  const PhaseFractionList& f = p_.phasefractions();
  if 
  (     
    (f.find(fieldname)!=f.end())
    && 
    (get<0>(fieldinfo)==scalarField) 
  )
  {
    BC["type"]="fixedValue";
    std::ostringstream entry;
    entry << "uniform "<<f.find(fieldname)->second;
    BC["value"]=entry.str();
    return true;
  }
  else 
    return false;
}

}

SuctionInletBC::SuctionInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void SuctionInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("pressureInletOutletVelocity");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("totalPressure");
      BC["p0"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
      BC["U"]=OFDictData::data(p_.UName());
      BC["phi"]=OFDictData::data(p_.phiName());
      BC["rho"]=OFDictData::data(p_.rhoName());
      BC["psi"]=OFDictData::data(p_.psiName());
      BC["gamma"]=OFDictData::data(p_.gamma());
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    ( 
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}

VelocityInletBC::VelocityInletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void VelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  BC["type"]=OFDictData::data("fixedValue");
  BC["value"]=OFDictData::data("uniform ( "
    +lexical_cast<std::string>(p_.velocity()(0))+" "
    +lexical_cast<std::string>(p_.velocity()(1))+" "
    +lexical_cast<std::string>(p_.velocity()(2))+" )");
}

void VelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      setField_U(BC);
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if ( (field.first=="k") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else if ( (field.first=="omega") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1.0";
    }
    else if ( (field.first=="nut") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else if ( (field.first=="nuSgs") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]="uniform 1e-10";
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)
	  ))
	throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}

TurbulentVelocityInletBC::TurbulentVelocityInletBC
(
  OpenFOAMCase& c,
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  Parameters const& p
)
: VelocityInletBC(c, patchName, boundaryDict, p),
  p_(p)
{
}


void TurbulentVelocityInletBC::setField_U(OFDictData::dict& BC) const
{
  std::string Uvec="( "
    +lexical_cast<std::string>(p_.velocity()(0))+" "
    +lexical_cast<std::string>(p_.velocity()(1))+" "
    +lexical_cast<std::string>(p_.velocity()(2))+" )";
    
  BC["type"]="inflowGenerator<"+p_.structureType()+">";
  BC["Umean"]="uniform "+Uvec;
  
  double L=p_.mixingLength();
  BC["L"]="uniform ( "
    +lexical_cast<string>(L)+" 0 0 "
    +lexical_cast<string>(L)+" 0 "
    +lexical_cast<string>(L)+" )";

  double R=pow(p_.turbulenceIntensity()*norm(p_.velocity(),2), 2);
  BC["R"]="uniform ( "
    +lexical_cast<string>(R)+" 0 0 "
    +lexical_cast<string>(R)+" 0 "
    +lexical_cast<string>(R)+" )";

  BC["value"]="uniform "+Uvec;
}

void TurbulentVelocityInletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  VelocityInletBC::addIntoFieldDictionaries(dictionaries);
  
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.addListIfNonexistent("libs").push_back( OFDictData::data("\"libinflowGeneratorBC.so\"") );
}

void TurbulentVelocityInletBC::initInflowBC(const boost::filesystem::path& location) const
{
  OFDictData::dictFile inflowProperties;
  
  OFDictData::list& initializers = inflowProperties.addListIfNonexistent("initializers");
  
  OFDictData::dict d;
  d["Ubulk"]=arma::norm(p_.velocity(), 2);
  d["D"]=2.0;
  d["patchName"]=patchName_;
  
  initializers.push_back( "pipeFlow" );
  initializers.push_back( d );
  
  // then write to file
  inflowProperties.write( location / "constant" / "inflowProperties" );
  
  OFcase().executeCommand(location, "initInflowGenerator");
}
  
PressureOutletBC::PressureOutletBC
(
  OpenFOAMCase& c, 
  const std::string& patchName, 
  const OFDictData::dict& boundaryDict, 
  const Parameters& p
)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
}

void PressureOutletBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC=dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("inletOutlet");
      BC["inletValue"]=OFDictData::data("uniform ( 0 0 0 )");
      BC["value"]=OFDictData::data("uniform ( 0 0 0 )");
    }
    else if ( 
      ( (field.first=="p") || (field.first=="pd") || (field.first=="p_rgh") )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.pressure()));
    }
    else if ( (field.first=="rho") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform "+lexical_cast<std::string>(p_.rho()) );
    }
    else if 
    (
      (
	(field.first=="k") ||
	(field.first=="epsilon") ||
	(field.first=="omega") ||
	(field.first=="nut") ||
	(field.first=="nuSgs") ||
	(field.first=="nuTilda")
      )
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    else
    {
      if (!(
	  noMeshMotion.addIntoFieldDictionary(field.first, field.second, BC)
/*	  ||
	  p_.phasefractions()->addIntoFieldDictionary(field.first, field.second, BC)*/
	  ))
	throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}


MeshMotionBC::MeshMotionBC()
{
}

MeshMotionBC::~MeshMotionBC()
{
}

void MeshMotionBC::addIntoDictionaries(OFdicts& dictionaries) const
{}

bool NoMeshMotion::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
    if 
    ( 
      ((fieldname=="displacement")||(fieldname == "motionU"))
      && 
      (get<0>(fieldinfo)==vectorField) 
    )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform (0 0 0)");
      return true;
    }
    else 
      return false;
}

MeshMotionBC* NoMeshMotion::clone() const
{
  return new NoMeshMotion(*this);
}

NoMeshMotion noMeshMotion;



CAFSIBC::CAFSIBC(const Parameters& p)
: p_(p)
{
}

CAFSIBC::~CAFSIBC()
{
}

void CAFSIBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict.getList("libs").push_back( OFDictData::data("\"libFEMDisplacementBC.so\"") );
}

bool CAFSIBC::addIntoFieldDictionary(const string& fieldname, const FieldInfo& fieldinfo, OFDictData::dict& BC) const
{
  if (fieldname == "motionU")
  {
    BC["prescribeMotionVelocity"] = OFDictData::data(true);
  }
  if ( (fieldname == "pointDisplacement") || (fieldname == "motionU") )
  {
    BC["type"]= OFDictData::data("FEMDisplacement");
    BC["FEMCaseDir"]=  OFDictData::data(std::string("\"")+p_.FEMScratchDir().c_str()+"\"");
    BC["pressureScale"]=  OFDictData::data(p_.pressureScale());
    BC["minPressure"]=  OFDictData::data(p_.clipPressure());
    BC["nSmoothIter"]=  OFDictData::data(4);
    BC["wallCollisionCheck"]=  OFDictData::data(true);
    if (p_.oldPressure().get())
    {
      std::ostringstream oss;
      oss<<"uniform "<<*p_.oldPressure();
      BC["oldPressure"] = OFDictData::data(oss.str());
    }
    BC["value"]=OFDictData::data("uniform (0 0 0)");
    
    OFDictData::list relaxProfile;
    if (p_.relax().which()==0)
    {
      OFDictData::list cp;
      cp.push_back(0.0);
      cp.push_back( boost::get<double>(p_.relax()) );
      relaxProfile.push_back( cp );
    }
    else
    {
      BOOST_FOREACH(const RelaxProfile::value_type& rp,  boost::get<RelaxProfile>(p_.relax()) )
      {
	OFDictData::list cp;
	cp.push_back(rp.first);
	cp.push_back(rp.second);
	relaxProfile.push_back(cp);
      }
    }
    BC["relax"]=  relaxProfile;
    
    return true;
  }
  return false;
}

MeshMotionBC* CAFSIBC::clone() const
{
  return new CAFSIBC(*this);
}


WallBC::WallBC(OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict, Parameters const& p)
: BoundaryCondition(c, patchName, boundaryDict),
  p_(p)
{
  type_="wall";
}

void WallBC::addIntoDictionaries(OFdicts& dictionaries) const
{
  p_.meshmotion()->addIntoDictionaries(dictionaries);
  BoundaryCondition::addIntoDictionaries(dictionaries);
}

void WallBC::addIntoFieldDictionaries(OFdicts& dictionaries) const
{
  BoundaryCondition::addIntoFieldDictionaries(dictionaries);
  
  BOOST_FOREACH(const FieldList::value_type& field, OFcase().fields())
  {
    OFDictData::dict& BC = dictionaries.addFieldIfNonexistent("0/"+field.first, field.second)
      .subDict("boundaryField").subDict(patchName_);
    
    // velocity
    if ( (field.first=="U") && (get<0>(field.second)==vectorField) )
    {
      BC["type"]=OFDictData::data("fixedValue");
      BC["value"]=OFDictData::data("uniform ("+toStr(p_.wallVelocity())+")");
    }
    
    // pressure
    else if ( (field.first=="p") && (get<0>(field.second)==scalarField) )
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    // turbulence quantities, should be handled by turbulence model
    else if ( 
      ( (field.first=="k") || (field.first=="omega") || (field.first=="nut") ) 
      && 
      (get<0>(field.second)==scalarField) 
    )
    {
      OFcase().get<turbulenceModel>("turbulenceModel")->addIntoFieldDictionary(field.first, field.second, BC);
    }
    
    // any other scalar field
    else if (get<0>(field.second)==scalarField)
    {
      BC["type"]=OFDictData::data("zeroGradient");
    }
    
    else
    {
      if (!p_.meshmotion()->addIntoFieldDictionary(field.first, field.second, BC))
	throw insight::Exception("Don't know how to handle field \""+field.first+"\" of type "+lexical_cast<std::string>(get<0>(field.second)) );
    }
  }
}

void WallBC::addOptionsToBoundaryDict(OFDictData::dict& bndDict) const
{
  BoundaryCondition::addOptionsToBoundaryDict(bndDict);
}

}

