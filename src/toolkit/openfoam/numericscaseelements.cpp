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


std::vector<int> factors(int n)
{
  // do a prime factorization of n
  
  std::vector<int> facs;
  int z = 2;

  while (z * z <= n)
  {
      if (n % z == 0)
      {
	  facs.push_back(z);
	  n /= z;
      }
      else
      {
	  z++;
      }
  }

  if (n > 1)
  {
      facs.push_back( n );
  }

  return facs;
}

std::vector<int> combinefactors
(
  std::vector<int> facs,
  const std::tuple<int,int,int>& po
)
{
//   cout<<"GOT:"<<endl;
//   BOOST_FOREACH(int f, facs) cout<<f<<" "; cout<<endl;
  
  // count locked directions (without decomposition)
  int n_lock=0;
  if (std::get<0>(po)<=0.0) n_lock++;
  if (std::get<1>(po)<=0.0) n_lock++;
  if (std::get<2>(po)<=0.0) n_lock++;
// if (n_lock==0) *static_cast<int*>(0)=23;
  // if less than 3 factors or directions are locked: extend with ones
  int n_add=std::max(3-int(facs.size()), n_lock);
  for (int k=0;k<n_add;k++)
    facs.push_back(1);
  
  // bring factors into descending order
  sort(facs.begin(), facs.end());
  std::reverse(facs.begin(), facs.end());
//   BOOST_FOREACH(int f, facs) cout<<f<<" "; cout<<endl;
  
  // get initial number which was factored
  double totprod=1.0;
  BOOST_FOREACH(int f, facs) totprod*=double(f);
  
  double potot=std::get<0>(po)+std::get<1>(po)+std::get<2>(po);
  std::vector<double> pof = list_of 
    (double(std::get<0>(po))/potot)
    (double(std::get<1>(po))/potot)
    (double(std::get<2>(po))/potot);
//   BOOST_FOREACH(double f, pof) cout<<f<<" "; cout<<endl;
  
  std::vector<std::size_t> pof_sorti(pof.size());
  std::iota(pof_sorti.begin(), pof_sorti.end(), 0);
  std::sort(pof_sorti.begin(), pof_sorti.end(), [&pof](std::size_t left, std::size_t right)
  {
      return pof[left] > pof[right];
  });
//   BOOST_FOREACH(int f, pof_sorti) cout<<f<<" "; cout<<endl;
  
  std::vector<int> nf(3);
  int j=0;
  
  for (int i=0; i<3; i++)
  {
    int dir_idx=pof_sorti[i];
//     cout<<"cumulating for "<<dir_idx<<endl;
    double req_frac=pof[dir_idx];
    int cf=facs[j++];
//     cout<<"cf="<<(log(cf)/log(totprod))<<endl;
    while (j<facs.size()-(2-i) && (cf>=0.0) && ( (log(cf)/log(totprod)) < req_frac) )
    {
//       double frac=double(cf)/totprod;
//       if ( frac<req_frac )
// 	break;
//       else
      {
	cf*=facs[j++];
// 	cout<<"inc: "<<cf<<endl;
// 	cout<<"cf(*)="<<(log(cf)/log(totprod))<<endl;
      }
    }
//     cout<<cf<<endl;
    nf[dir_idx]=cf;
  }
  
//   cout<<"combined"<<endl;
//     BOOST_FOREACH(int i, nf)
//       cout <<" > "<<i<<endl;
      
  return nf;
}

void setDecomposeParDict
(
  OFdicts& dictionaries, int np, const std::string& method, 
  const std::tuple<int,int,int>& po
)
{
  OFDictData::dict& decomposeParDict=dictionaries.addDictionaryIfNonexistent("system/decomposeParDict");
 
// #warning hack for testing
  std::vector<int> ns=combinefactors(factors(np), po);
  cout<<"decomp "<<np<<": "<<ns[0]<<" "<<ns[1]<<" "<<ns[2]<<endl;
  decomposeParDict["numberOfSubdomains"]=np;
  
  decomposeParDict["method"]=method; //"hierarchical";
  
  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    coeffs["order"]="xyz";
    decomposeParDict["hierarchicalCoeffs"]=coeffs;
  }
  
  {
    OFDictData::dict coeffs;
    coeffs["n"]=OFDictData::vector3(ns[0], ns[1], ns[2]);
    coeffs["delta"]=0.001;
    decomposeParDict["simpleCoeffs"]=coeffs;
  }
}


FVNumerics::FVNumerics(OpenFOAMCase& c, Parameters const& p)
: OpenFOAMCaseElement(c, "FVNumerics"),
  p_(p),
  isCompressible_(false)
{
}

void FVNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  // setup structure of dictionaries
  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
//   controlDict["endTime"]=p_.endTime();
  controlDict["deltaT"]=p_.deltaT();
  controlDict["adjustTimeStep"]=p_.adjustTimeStep();
  controlDict["maxCo"]=0.5;
  controlDict["startFrom"]="latestTime";
  controlDict["startTime"]=0.0;
  controlDict["stopAt"]="endTime";
  controlDict["endTime"]=p_.endTime();
  controlDict["writeControl"]=p_.writeControl();
  controlDict["writeInterval"]=p_.writeInterval();
  controlDict["purgeWrite"]=p_.purgeWrite();
  controlDict["writeFormat"]=p_.writeFormat();
  controlDict["writePrecision"]=8;
  controlDict["writeCompression"]="compressed";
  controlDict["timeFormat"]="general";
  controlDict["timePrecision"]=6;
  controlDict["runTimeModifiable"]=true;
  controlDict.addListIfNonexistent("libs");
  controlDict.addSubDictIfNonexistent("functions");

  controlDict.getList("libs").insertNoDuplicate( "\"libwriteData.so\"" );  

  OFDictData::dict wonow;
  wonow["type"]="writeData";
  wonow["fileName"]="\"wnow\"";
  controlDict.addSubDictIfNonexistent("functions")["writeData"]=wonow;
  
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

  setDecomposeParDict( dictionaries, p_.np(), p_.decompositionMethod(), p_.decompWeights() );
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

OFDictData::dict diagonalSolverSetup()
{
  OFDictData::dict d;
  d["solver"]="diagonal";
  return d;
}

OFDictData::dict stdAsymmSolverSetup(double tol, double reltol, int minIter)
{
  OFDictData::dict d;
  d["solver"]="PBiCG";
  d["preconditioner"]="DILU";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  if (minIter) d["minIter"]=minIter;
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
  d["nPreSweeps"]=2;
  d["nPostSweeps"]=2;
  d["cacheAgglomeration"]="on";
  d["agglomerator"]="faceAreaPair";
  d["nCellsInCoarsestLevel"]=500;
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
  pd["nPreSweeps"]=2;
  pd["nPostSweeps"]=2;
  pd["cacheAgglomeration"]="on";
  pd["agglomerator"]="faceAreaPair";
  pd["nCellsInCoarsestLevel"]=10;
  pd["mergeLevels"]=1;
  d["preconditioner"]=pd;
  return d;
}

OFDictData::dict smoothSolverSetup(double tol, double reltol, int minIter)
{
  OFDictData::dict d;
  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["nSweeps"]=1;
  if (minIter) d["minIter"]=minIter;
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
  controlDict["writeCompression"]="compressed";
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
  
  setDecomposeParDict(dictionaries, p_.np(), "hierarchical", p_.decompWeights() );
}

simpleFoamNumerics::simpleFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p),
  p_(p)
{
  c.addField("p", FieldInfo(scalarField, 	dimKinPressure, 	list_of(p_.pinternal()), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal().begin(), p_.Uinternal().end()), volField ) );
}
 
void simpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="simpleFoam";
  
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalCellLimitedGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalBlendedBy.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"libleastSquares2.so\"" );  
  
  OFDictData::dict fqmc;
  fqmc["type"]="faceQualityMarker";
  controlDict.addSubDictIfNonexistent("functions")["faceQualityMarker"]=fqmc;

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["R"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["p"]=0.3;
    relax["U"]=0.7;
    relax["k"]=0.7;
    relax["R"]=0.7;
    relax["omega"]=0.7;
    relax["epsilon"]=0.7;
    relax["nuTilda"]=0.7;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p"]=0.3;
    eqnRelax["U"]=0.7;
    eqnRelax["k"]=0.7;
    eqnRelax["R"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    eqnRelax["nuTilda"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=0;
  SIMPLE["pRefCell"]=0;
  SIMPLE["pRefValue"]=0.0;
  
  if ( (OFversion()>=210) && p_.checkResiduals() )
  {
    OFDictData::dict resCtrl;
    resCtrl["p"]=1e-4;
    resCtrl["U"]=1e-3;
    resCtrl["\"(k|epsilon|omega|nuTilda|R)\""]=1e-4;
    SIMPLE["residualControl"]=resCtrl;
  }
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  
//   std::string bgrads="leastSquares2"; 
  std::string bgrads="Gauss linear";
  if ( (OFversion()>=220) && !p_.hasCyclics() ) bgrads="pointCellsLeastSquares";
  
  grad["default"]=bgrads;
//   grad["grad(p)"]="Gauss linear";
//   grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
    
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";
  if (OFversion()<=160) suf=" localCellLimited "+bgrads+" UBlendingFactor"; else suf=" grad(U)";
  div["default"]="none"; //pref+"Gauss upwind";
  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV "+suf;
  div["div(phi,k)"]	=	pref+"Gauss linearUpwind "+suf;
  div["div(phi,omega)"]	=	pref+"Gauss upwind";
  div["div(phi,nuTilda)"]=	pref+"Gauss linearUpwind "+suf;
  div["div(phi,epsilon)"]=	pref+"Gauss upwind";
  div["div(phi,R)"]	=	pref+"Gauss upwind";
  div["div(R)"]="Gauss linear";
      
  div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

pimpleFoamNumerics::pimpleFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p),
  p_(p)
{
  c.addField("p", FieldInfo(scalarField, 	dimKinPressure, 	list_of(p_.pinternal()), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal().begin(), p_.Uinternal().end()), volField ) );
}
 
void pimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="pimpleFoam";
  controlDict["maxCo"]=p_.maxCo();
  controlDict["maxDeltaT"]=p_.maxDeltaT();
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["pFinal"]=GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=stdAsymmSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  PIMPLE["nCorrectors"]=p_.nCorrectors();
  PIMPLE["nOuterCorrectors"]=p_.nOuterCorrectors();
  PIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors();
  PIMPLE["pRefCell"]=0;
  PIMPLE["pRefValue"]=0.0;
  
  // ============ setup fvSchemes ================================
  
  // check if LES required
  bool LES=p_.forceLES();
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
    ddt["default"]="CrankNicolson 0.75"; // problems with pressureGradientSource (oscillations), if coefficient is =1!
  else
    ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  std::string bgrads="Gauss linear";
  if ( (OFversion()>=220) && !(p_.hasCyclics())) bgrads="pointCellsLeastSquares";
  grad["default"]=bgrads;
//   grad["grad(p)"]="Gauss linear";
//   grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";
  
  if (p_.nOuterCorrectors()>1)
  {
    // SIMPLE mode: add underrelaxation
    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OFversion()<210)
    {
      relax["p"]=0.3;
      relax["U"]=0.7;
      relax["k"]=0.7;
      relax["omega"]=0.7;
      relax["epsilon"]=0.7;
      relax["nuTilda"]=0.7;
    }
    else
    {
      OFDictData::dict fieldRelax, eqnRelax;
      fieldRelax["p"]=0.3;
      eqnRelax["U"]=0.7;
      eqnRelax["k"]=0.7;
      eqnRelax["omega"]=0.7;
      eqnRelax["epsilon"]=0.7;
      eqnRelax["nuTilda"]=0.7;
      relax["fields"]=fieldRelax;
      relax["equations"]=eqnRelax;
    }
  }
  
  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
      div["div(phi,U)"]="Gauss filteredLinear";
    div["div(phi,k)"]="Gauss linear";
    div["div(phi,nuTilda)"]="Gauss linear";
  }
  else
  {
    div["div(phi,U)"]="Gauss limitedLinearV 1";
    div["div(phi,k)"]="Gauss limitedLinear 1";
    div["div(phi,epsilon)"]="Gauss limitedLinear 1";
    div["div(phi,omega)"]="Gauss limitedLinear 1";
    div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
  }

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


potentialFreeSurfaceFoamNumerics::potentialFreeSurfaceFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p),
  p_(p)
{
  c.addField("p", FieldInfo(scalarField, 	dimKinPressure, 	list_of(0.0), volField ) );
  c.addField("p_gh", FieldInfo(scalarField, 	dimKinPressure, 	list_of(0.0), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
  
  if (OFversion()<230)
    throw insight::Exception("solver potentialFreeSurfaceFoam not available in selected OpenFOAM version!");
}
 
void potentialFreeSurfaceFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="potentialFreeSurfaceFoam";
  controlDict["maxCo"]=p_.maxCo();
  controlDict["maxDeltaT"]=p_.maxDeltaT();
    
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalCellLimitedGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalBlendedBy.so\"" );  
  
  OFDictData::dict fqmc;
  fqmc["type"]="faceQualityMarker";
  controlDict.addSubDictIfNonexistent("functions")["faceQualityMarker"]=fqmc;

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p_gh"]=GAMGSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["p_ghFinal"]=GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=stdAsymmSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  PIMPLE["nCorrectors"]=p_.nCorrectors();
  PIMPLE["nOuterCorrectors"]=p_.nOuterCorrectors();
  PIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors();
  PIMPLE["pRefCell"]=0;
  PIMPLE["pRefValue"]=0.0;
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  std::string bgrads="Gauss linear";
  if (OFversion()>=220) bgrads="pointCellsLeastSquares";
  grad["default"]=bgrads;
  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";
  
  if (p_.nOuterCorrectors()>1)
  {
    // SIMPLE mode: add underrelaxation
    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OFversion()<210)
    {
      relax["p_gh"]=0.3;
      relax["U"]=0.7;
      relax["k"]=0.7;
      relax["omega"]=0.7;
      relax["epsilon"]=0.7;
      relax["nuTilda"]=0.7;
    }
    else
    {
      OFDictData::dict fieldRelax, eqnRelax;
      fieldRelax["p_gh"]=0.3;
      eqnRelax["U"]=0.7;
      eqnRelax["k"]=0.7;
      eqnRelax["omega"]=0.7;
      eqnRelax["epsilon"]=0.7;
      eqnRelax["nuTilda"]=0.7;
      relax["fields"]=fieldRelax;
      relax["equations"]=eqnRelax;
    }
  }
  

  div["div(phi,U)"]="Gauss localBlendedBy UBlendingFactor linearUpwind grad(U) limitedLinearV 1";
  div["div(phi,k)"]="Gauss linearUpwind grad(k)";
  div["div(phi,epsilon)"]="Gauss linearUpwind grad(epsilon)";
  div["div(phi,omega)"]="Gauss upwind";
  div["div(phi,nuTilda)"]="Gauss linearUpwind grad(nuTilda)";
  div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p_gh"]="";
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
: FVNumerics(c, p),
  p_(p)
{
  c.addField("p", FieldInfo(scalarField, 	dimPressure, 		list_of(p_.pamb()), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
  c.addField("rho", FieldInfo(scalarField, 	dimDensity, 		list_of(p_.rhoamb()), volField ) );
}
 
void cavitatingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName();
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
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0, 1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTilda"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("PISO");
  SIMPLE["nCorrectors"]=OFDictData::data( 2 );
  SIMPLE["nNonOrthogonalCorrectors"]=OFDictData::data( 0 );
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  std::string bgrads="Gauss linear";
  if (OFversion()>=220) bgrads="pointCellsLeastSquares";
  grad["default"]=bgrads;
  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  
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
: FVNumerics(c, p),
  p_(p)
{
  if (OFversion()<=160)
    pname_="pd";
  else
    pname_="p_rgh";
  
  alphaname_="alpha1";
  if (OFversion()>=230)
    alphaname_="alpha.phase1";
  
  // create pressure field to enable mapping from single phase cases
  c.addField("p", 		FieldInfo(scalarField, 	dimPressure, 	list_of(0.0), 		volField ) );
  
  c.addField(pname_, 		FieldInfo(scalarField, 	dimPressure, 	list_of(0.0), 		volField ) );
  c.addField("U", 		FieldInfo(vectorField, 	dimVelocity, 	list_of(0.0)(0.0)(0.0), volField ) );
  c.addField(alphaname_,	FieldInfo(scalarField, dimless, 	list_of(0.0),		volField ) );
}

const double cAlpha=0.25; // use low compression by default, since split of interface at boundaries of refinement zones otherwise
const double icAlpha=0.1;
 
void interFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="interFoam";

  controlDict["maxDeltaT"]=1.0;
  controlDict["maxCo"]=0.4;
  controlDict["maxAlphaCo"]=0.2;
  if (p_.implicitPressureCorrection())
  {
    controlDict["maxCo"]=10;
    controlDict["maxAlphaCo"]=5;
  }

  OFDictData::list fol;
  fol.push_back("\"libnumericsFunctionObjects.so\"");

  OFDictData::dict fqmc;
  fqmc["type"]="faceQualityMarker";
  fqmc["functionObjectLibs"]= fol;
  fqmc["lowerNonOrthThreshold"]=45.0;
  fqmc["upperNonOrthThreshold"]=60.0;
  controlDict.addSubDictIfNonexistent("functions")["fqm"]=fqmc;

//   OFDictData::dict ifmc;
//   ifmc["type"]="interfaceMarker";
//   ifmc["functionObjectLibs"]= fol;
//   ifmc["phaseFieldName"]=alphaname_;
//   OFDictData::list bfn;
//   bfn.push_back("interfaceBlendingFactor");
//   ifmc["blendingFieldNames"]=bfn;
//   controlDict.addSubDictIfNonexistent("functions")["ifm"]=ifmc;

  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalBlendedBy.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalCellLimitedGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalFaceLimitedGrad.so\"" );  
//   controlDict.getList("libs").insertNoDuplicate( "\"libleastSquares2.so\"" );  
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["pcorr"]=stdSymmSolverSetup(1e-7, 0.01);
  solvers[pname_]=GAMGPCGSolverSetup(1e-7, 0.01);
  solvers[pname_+"Final"]=GAMGPCGSolverSetup(1e-7, 0.0);
  
  solvers["U"]=smoothSolverSetup(1e-8, 0);
  solvers["k"]=smoothSolverSetup(1e-8, 0);
  solvers["omega"]=smoothSolverSetup(1e-12, 0, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0);
  
  solvers["UFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["kFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["omegaFinal"]=smoothSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=smoothSolverSetup(1e-10, 0);
  solvers["nuTildaFinal"]=smoothSolverSetup(1e-10, 0);

  double Urelax=0.7, prelax=1.0, turbrelax=0.95;
  if (p_.implicitPressureCorrection())
  {
    prelax=0.3;
    turbrelax=0.9;
  }
  
  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  if (OFversion()<210)
  {
    relax["U"]=Urelax;
    if (turbrelax<1.) relax["\"(k|omega|epsilon|nuTilda).*\""]=turbrelax;
    if (prelax<1.) relax["\"(p|pd|p_rgh)\""]=prelax;
  }
  else
  {
    OFDictData::dict fieldRelax, eqnRelax;
    eqnRelax["\"U.*\""]=Urelax;
    if (turbrelax<1.) eqnRelax["\"(k|omega|epsilon|nuTilda).*\""]=turbrelax;
    if (prelax<1.) fieldRelax["\"(p|pd|p_rgh).*\""]=prelax;
    
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }
  
  std::string solutionScheme("PISO");
  if (OFversion()>=210) solutionScheme="PIMPLE";
  OFDictData::dict& SOL=fvSolution.addSubDictIfNonexistent(solutionScheme);
  SOL["momentumPredictor"]=true;
  SOL["nNonOrthogonalCorrectors"]=1;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=4;
  SOL["cAlpha"]=cAlpha;
  SOL["nCorrectors"]=2;
  SOL["nOuterCorrectors"]=1;
  
  if (p_.implicitPressureCorrection())
  {
    SOL["nCorrectors"]=1;
    SOL["nOuterCorrectors"]=50;
    
    OFDictData::dict tol;
    tol["tolerance"]=1e-4;
    tol["relTol"]=0.0;
    
    OFDictData::dict residualControl;
    residualControl["\"(p|p_rgh|pd).*\""]=tol;
    residualControl["\"U.*\""]=tol;
    residualControl["\"(k|epsilon|omega|nuTilda)\""]=tol;
    
    SOL["residualControl"]=residualControl;
  }
  
  // ============ setup fvSchemes ================================
  
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");
  
  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="Euler";
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  
//   std::string bgrads="leastSquares2"; 
  std::string bgrads="Gauss linear";
 // if (OFversion()>=220) bgrads="pointCellsLeastSquares";
  
  grad["default"]=bgrads; //"faceLimited leastSquares 1"; // plain limiter gives artifacts ("schlieren") near (above and below) waterline
//   grad["grad(p_rgh)"]="Gauss linear";
  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  grad["grad("+alphaname_+")"]="localFaceLimited "+bgrads+" UBlendingFactor";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  if (OFversion()==160) 
    suf=bgrads;
  else 
    suf="grad(U)";
  div["div(rho*phi,U)"]		= "Gauss linearUpwindV "+suf; //localBlendedBy interfaceBlendingFactor linearUpwindV "+suf+" upwind";
  div["div(rhoPhi,U)"]		= "Gauss linearUpwindV "+suf; //localBlendedBy interfaceBlendingFactor linearUpwindV "+suf+" upwind"; // for interPhaseChangeFoam
//   div["div(phi,alpha)"]		= "Gauss localBlendedBy UBlendingFactor upwind vanLeer";
//   div["div(phirb,alpha)"]	= "Gauss localBlendedBy UBlendingFactor upwind linear"; //interfaceCompression";
  div["div(phi,alpha)"]		= "Gauss vanLeer";
  div["div(phirb,alpha)"]	= "Gauss linear"; //interfaceCompression";
  div["div(phi,k)"]		= "Gauss linearUpwind "+suf;
  div["div(phi,epsilon)"]	= "Gauss linearUpwind "+suf;
  div["div(phi,omega)"]		= "Gauss upwind";
  div["div(phi,nuTilda)"]	= "Gauss linearUpwind "+suf;
  div["div(phi,R)"]		= "Gauss linearUpwind "+suf;
  div["div(R)"]			= "Gauss linear";
  if (OFversion()>=210)
    div["div((muEff*dev(T(grad(U)))))"]="Gauss linear";
  else
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["laplacian(rAUf,pcorr)"]="Gauss linear limited 0.66";
  laplacian["laplacian((1|A(U)),pcorr)"]="Gauss linear limited 0.66";
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["interpolate(U)"]="pointLinear";
  interpolation["default"]="linear"; //"pointLinear"; // OF23x: pointLinear as default creates artifacts at parallel domain borders!

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired[pname_]="";
  fluxRequired["pcorr"]="";
  fluxRequired["alpha"]="";
  fluxRequired[alphaname_]="";
}

OFDictData::dict stdMULESSolverSetup(double tol, double reltol, bool LTS)
{
  OFDictData::dict d;
  
  d["nAlphaCorr"]=1;
  d["nAlphaSubCycles"]=1;
  d["cAlpha"]=cAlpha;
  d["icAlpha"]=icAlpha;

  d["MULESCorr"]=true;
  d["nLimiterIter"]=10;
  d["alphaApplyPrevCorr"]=LTS;

  d["solver"]="smoothSolver";
  d["smoother"]="symGaussSeidel";
  d["tolerance"]=tol;
  d["relTol"]=reltol;
  d["minIter"]=1;
  d["maxIter"]=100;

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
  
  //double maxCo=10.0, maxAlphaCo=5.0;
  double maxCo=10.0, maxAlphaCo=2.0;
  bool momentumPredictor=false;

  controlDict["maxAlphaCo"]=maxAlphaCo;
  controlDict["maxCo"]=maxCo;
 
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  if (OFversion()>=230)
    solvers["\"alpha.*\""]=stdMULESSolverSetup(1e-8, 0.0, true);


  std::string solutionScheme("PIMPLE");
  OFDictData::dict& SOL=fvSolution.addSubDictIfNonexistent(solutionScheme);
  SOL["momentumPredictor"]=momentumPredictor;
  SOL["nCorrectors"]=2;
  SOL["nNonOrthogonalCorrectors"]=0;
  SOL["nAlphaCorr"]=1;
  SOL["nAlphaSubCycles"]=1;
  SOL["cAlpha"]=cAlpha;
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
  ddt["default"]="localEuler rDeltaT";
  
//   OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
}

interPhaseChangeFoamNumerics::interPhaseChangeFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: interFoamNumerics(c, p),
  p_(p)
{
}
 
void interPhaseChangeFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  interFoamNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]=p_.solverName();

  // ============ setup controlDict ================================

  controlDict["maxDeltaT"]=1.0;

  controlDict["maxCo"]=0.4;
  controlDict["maxAlphaCo"]=0.2;
  if (p_.implicitPressureCorrection())
  {
    controlDict["maxCo"]=5;
    controlDict["maxAlphaCo"]=2.5;
  }

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  
  OFDictData::dict alphasol = stdMULESSolverSetup(1e-10, 0.0, false);
  solvers["\"alpha.*\""]=alphasol;

}

reactingFoamNumerics::reactingFoamNumerics(OpenFOAMCase& c, const Parameters& p)
: FVNumerics(c, p),
  p_(p)
{
  isCompressible_=true;
  
  if (OFversion() < 230)
    throw insight::Exception("reactingFoamNumerics currently supports only OF >=230");
  
  c.addField("p", FieldInfo(scalarField, 	dimPressure, 	list_of(1e5), volField ) );
  c.addField("U", FieldInfo(vectorField, 	dimVelocity, 		list_of(0.0)(0.0)(0.0), volField ) );
  c.addField("T", FieldInfo(scalarField, 	dimTemperature,		list_of(300.0), volField ) );
}

void reactingFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  insight::FVNumerics::addIntoDictionaries(dictionaries);
    
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="reactingFoam";
  controlDict["maxCo"]=p_.maxCo();
  controlDict["maxDeltaT"]=p_.maxDeltaT();
  
  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["\"rho.*\""]=diagonalSolverSetup();
  solvers["p"]=stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["k"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["h"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["omega"]=stdAsymmSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=stdAsymmSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=stdAsymmSolverSetup(1e-8, 0.1);
  
  solvers["pFinal"]=stdSymmSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=stdAsymmSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["hFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=stdAsymmSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=stdAsymmSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=stdAsymmSolverSetup(1e-8, 0);

  solvers["Yi"]=stdAsymmSolverSetup(1e-8, 0);

  OFDictData::dict& PIMPLE=fvSolution.addSubDictIfNonexistent("PIMPLE");
  PIMPLE["nCorrectors"]=p_.nCorrectors();
  PIMPLE["nOuterCorrectors"]=p_.nOuterCorrectors();
  PIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors();
  PIMPLE["pRefCell"]=0;
  PIMPLE["pRefValue"]=1e5;
  
  // ============ setup fvSchemes ================================
  
  // check if LES required
  bool LES=p_.forceLES();
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
  
  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");
  std::string bgrads="Gauss linear";
  if (OFversion()>=220) bgrads="pointCellsLeastSquares";
  grad["default"]=bgrads;
  grad["grad(U)"]="cellMDLimited "+bgrads+" 1";
  
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";
  
  if (p_.nOuterCorrectors()>1)
  {
    // SIMPLE mode: add underrelaxation
    OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
    if (OFversion()<210)
    {
      relax["p"]=0.3;
      relax["U"]=0.7;
      relax["k"]=0.7;
      relax["h"]=0.7;
      relax["Yi"]=0.7;
      relax["omega"]=0.7;
      relax["nuTilda"]=0.7;
      relax["epsilon"]=0.7;
    }
    else
    {
      OFDictData::dict fieldRelax, eqnRelax;
      fieldRelax["p"]=0.3;
      eqnRelax["U"]=0.7;
      eqnRelax["k"]=0.7;
      eqnRelax["h"]=0.7;
      eqnRelax["Yi"]=0.7;
      eqnRelax["omega"]=0.7;
      eqnRelax["nuTilda"]=0.7;
      eqnRelax["epsilon"]=0.7;
      relax["fields"]=fieldRelax;
      relax["equations"]=eqnRelax;
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
    div["div(phi,U)"]="Gauss linearUpwindV grad(U)";
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

reactingParcelFoamNumerics::reactingParcelFoamNumerics(OpenFOAMCase& c, const Parameters& p)
: reactingFoamNumerics(c, p)
{

}

void reactingParcelFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  reactingFoamNumerics::addIntoDictionaries(dictionaries);
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

magneticFoamNumerics::magneticFoamNumerics(OpenFOAMCase& c, Parameters const& p)
: FVNumerics(c, p)
{
  c.addField("psi", FieldInfo(scalarField, 	dimCurrent, 	list_of(0.0), volField ) );
}
 
void magneticFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);
  
  // ============ setup controlDict ================================
  
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  controlDict["application"]="magneticFoam";
  
  controlDict.getList("libs").insertNoDuplicate( "\"libnumericsFunctionObjects.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalLimitedSnGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalCellLimitedGrad.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"liblocalBlendedBy.so\"" );  
  controlDict.getList("libs").insertNoDuplicate( "\"libleastSquares2.so\"" );  
  
  OFDictData::dict fqmc;
  fqmc["type"]="faceQualityMarker";
  controlDict.addSubDictIfNonexistent("functions")["faceQualityMarker"]=fqmc;

  // ============ setup fvSolution ================================
  
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["psi"]=stdSymmSolverSetup(1e-7, 0.0);

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

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
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
  
  std::string bgrads="Gauss linear";
//   if (OFversion()>=220) bgrads="pointCellsLeastSquares";
  grad["default"]=bgrads;
    
  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  div["default"]="none";
      
  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 0.66";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["T"]="";

  OFDictData::dict& transportProperties=dictionaries.addDictionaryIfNonexistent("constant/transportProperties");
  transportProperties.addListIfNonexistent("magnets");

}

}

