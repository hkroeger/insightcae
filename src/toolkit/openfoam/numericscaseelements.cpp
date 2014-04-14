/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
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
  
  
FVNumerics::FVNumerics(OpenFOAMCase& c, Parameters const& p)
: OpenFOAMCaseElement(c, "FVNumerics"),
  p_(p)
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
  controlDict["writePrecision"]=8;
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


MeshingNumerics::MeshingNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p)
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
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1);
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
  int np1, np2, np3; 
  np1=std::max(1, int(pow(p_.np(), 1./3.)));
  np2=std::max(1, int(pow(double(p_.np()/np1), 1./2.)));
  np3=p_.np()/np1/np2;
  cout<<"decomp "<<p_.np()<<": "<<np1<<" "<<np2<<" "<<np3<<endl;
  decomposeParDict["numberOfSubdomains"]=p_.np();
  decomposeParDict["method"]="hierarchical";
  OFDictData::dict coeffs;
  coeffs["n"]=OFDictData::vector3(np1,np2,np3);
  coeffs["delta"]=0.001;
  coeffs["order"]="xyz";
  decomposeParDict["hierarchicalCoeffs"]=coeffs;
  
}

simpleFoamNumerics::simpleFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p)
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
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1);
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
: FVNumerics(c, p),
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
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0.1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["pFinal"]=GAMGPCGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=stdAsymmSolverSetup(1e-14, 0);
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
  if (p_.LES())
    ddt["default"]="backward";
  else
    ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  grad["default"]="Gauss linear";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss limitedLinear 1";

  if (p_.LES())
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
      div["div(phi,U)"]="Gauss linear";
  }
  else
   div["div(phi,U)"]="Gauss limitedLinearV 1";

  if (OFversion()>=230)
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }
  else if (OFversion()>=210)
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
: simpleFoamNumerics(c, p),
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


cavitatingFoamNumerics::cavitatingFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p)
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
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0);
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

interFoamNumerics::interFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p)
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
  solvers["omega"]=smoothSolverSetup(1e-12, 0);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0);
  
  solvers["UFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["kFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["omegaFinal"]=smoothSolverSetup(1e-14, 0);
  solvers["epsilonFinal"]=smoothSolverSetup(1e-10, 0);

  double Urelax=0.7;
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["U"]=Urelax;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    eqnRelax["U"]=Urelax;
    relax["fields"]=fieldRelax;
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
  div["div(phi,omega)"]="Gauss upwind";
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

OFDictData::dict stdMULESSolverSetup(double tol, double reltol)
{
  OFDictData::dict d;
  
  d["nAlphaCorr"]=2;
  d["nAlphaSubCycles"]=1;
  d["cAlpha"]=1;
  d["icAlpha"]=0;

  d["MULESCorr"]=true;
  d["nLimiterIter"]=10;
  d["alphaApplyPrevCorr"]=true;

  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;

  return d;
}

LTSInterFoamNumerics::LTSInterFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: interFoamNumerics(c, p)
{
}

void LTSInterFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="LTSInterFoam";
  controlDict["endTime"]=100000.0;
  controlDict["deltaT"]=1;

//   controlDict["maxCo"]=0.5;
//   controlDict["maxAlphaCo"]=50.;
//   controlDict["maxDeltaT"]=1.0;
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  if (OFversion()>=230)
    solvers["\"alpha1.*\""]=stdMULESSolverSetup();
//   solvers["pcorr"]=GAMGPCGSolverSetup(1e-6, 0.0);
//   solvers[pname_]=GAMGPCGSolverSetup(1e-7, 0.01);
//   solvers[pname_+"Final"]=GAMGPCGSolverSetup(1e-7, 0.0);
//   
//   solvers["U"]=smoothSolverSetup(1e-8, 0);
//   solvers["k"]=smoothSolverSetup(1e-8, 0);
//   solvers["omega"]=smoothSolverSetup(1e-8, 0);
//   solvers["epsilon"]=smoothSolverSetup(1e-8, 0);
//   
//   solvers["UFinal"]=smoothSolverSetup(1e-10, 0);
//   solvers["kFinal"]=smoothSolverSetup(1e-10, 0);
//   solvers["omegaFinal"]=smoothSolverSetup(1e-10, 0);
//   solvers["epsilonFinal"]=smoothSolverSetup(1e-10, 0);

//   OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
//   if (OFversion()<210)
//   {
//     relax["U"]=0.7;
//   }
//   else
//   {
//     OFDictData::dict /*fieldRelax,*/ eqnRelax;
//     eqnRelax["U"]=0.7;
// //     relax["fields"]=fieldRelax;
//     relax["equations"]=eqnRelax;
//   }

  std::string solutionScheme("PIMPLE");
  OFDictData::dict& SOL=fvSolution.addSubDictIfNonexistent(solutionScheme);
  SOL["momentumPredictor"]=true;
  SOL["nCorrectors"]=2;
  SOL["nNonOrthogonalCorrectors"]=1;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=1;
  SOL["cAlpha"]=1.0;
  SOL["maxCo"]=0.9;
  SOL["maxAlphaCo"]=0.2;
  SOL["rDeltaTSmoothingCoeff"]=0.05;
  SOL["rDeltaTDampingCoeff"]=0.5;
  SOL["nAlphaSweepIter"]=0;
  SOL["nAlphaSpreadIter"]=0;
  SOL["maxDeltaT"]=1;

  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="localEuler rDeltaT";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  //grad["grad(p_rgh)"]="Gauss linear";
  grad["default"]="cellLimited leastSquares 1";
  grad["grad(U)"]="cellLimited leastSquares 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  if (OFversion()==160) 
    suf="cellLimited leastSquares 1";
  else 
    suf="grad(U)";
  div["div(phi,k)"]="Gauss linearUpwind "+suf;
  div["div(phi,epsilon)"]="Gauss linearUpwind "+suf;
  div["div(phi,omega)"]="Gauss linearUpwind "+suf;
  div["div(phi,R)"]="Gauss linearUpwind "+suf;
  div["div(phi,nuTilda)"]="Gauss linearUpwind "+suf;

}

interPhaseChangeFoamNumerics::interPhaseChangeFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: interFoamNumerics(c, p)
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

}

