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

#include "incompressiblenumericscaseelements.h"
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


defineType(simpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(simpleFoamNumerics);

simpleFoamNumerics::simpleFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
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
  solvers["p"]=GAMGPCGSolverSetup(1e-7, 0.01);
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

  if ( (OFversion()>=210) && p_.checkResiduals )
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

  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";

  div["default"]="none";

  auto gradNameOrScheme = [&](const std::string& key) -> std::string {
    if (OFversion()>=220)
      return key;
    else
      return fvSchemes.subDict("gradSchemes").getString(key);
  };

  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV "+gradNameOrScheme("limitedGrad");

  div["div(phi,nuTilda)"]       = "Gauss linearUpwind "+gradNameOrScheme("grad(nuTilda)");
  div["div(phi,k)"]		= "Gauss linearUpwind "+gradNameOrScheme("grad(k)");
  div["div(phi,epsilon)"]	= "Gauss linearUpwind "+gradNameOrScheme("grad(epsilon)");
  div["div(phi,omega)"]		= "Gauss linearUpwind "+gradNameOrScheme("grad(omega)");
  div["div(phi,R)"]             = "Gauss upwind";
  div["div(R)"]="Gauss linear";

  div["div((nuEff*dev(grad(U).T())))"]="Gauss linear"; // kOmegaSST2
  if (OFversion()>=300)
  {
    div["div((nuEff*dev2(T(grad(U)))))"]="Gauss linear";
    div["div((nu*dev2(T(grad(U)))))"]="Gauss linear"; // LRR
  }
  else
  {
    div["div((nuEff*dev(T(grad(U)))))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";
//   laplacian["laplacian(1,p)"]="Gauss linear limited 0.33";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

ParameterSet simpleFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}




defineType(pimpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(pimpleFoamNumerics);

pimpleFoamNumerics::pimpleFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
}



void pimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // check if LES required
  bool LES=p_.forceLES;
  try
  {
    const turbulenceModel* tm=this->OFcase().get<turbulenceModel>("turbulenceModel");
    if (tm)
    {
      LES=LES || (tm->minAccuracyRequirement() >= turbulenceModel::AC_LES);
    }
  }
  catch (...)
  {
    insight::Warning("Warning: unhandled exception during LES check!");
  }

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (LES && (OFversion()<200))
   controlDict["application"]="pisoFoam";
  else
   controlDict["application"]="pimpleFoam";

  PIMPLESettings ps(p_.time_integration);
  ps.addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["p"]=GAMGSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0.1);

  solvers["pFinal"]=GAMGPCGSolverSetup(1e-8, 0.0); //GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=smoothSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=smoothSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=smoothSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=smoothSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=smoothSolverSetup(1e-8, 0);


  // ============ setup fvSchemes ================================


  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (LES)
  {
//     ddt["default"]="CrankNicolson 0.75"; // problems with pressureGradientSource (oscillations), if coefficient is =1!
    ddt["default"]="backward"; // channel with Retau=180 gets laminar with CrankNicholson...
  }
  else
  {
    ddt["default"]="Euler";
  }

  insertStandardGradientConfig(dictionaries);
//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

//  grad["default"]="cellLimited "+lqGradSchemeIfPossible()+" 1";
//  grad["grad(p)"]="Gauss linear";
//   grad["grad(U)"]="cellMDLimited "+bgrads+" 1";

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="Gauss linear";


  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
    if (p_.LESfilteredConvection)
      div["div(phi,U)"]="Gauss filteredLinear";
    else
      div["div(phi,U)"]="Gauss linear";
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
//   interpolation["interpolate(U)"]="pointLinear";
//   interpolation["interpolate(HbyA)"]="pointLinear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="limited 0.66";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

ParameterSet pimpleFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}


}
