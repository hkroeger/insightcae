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

#include "compressiblenumericscaseelements.h"
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




defineType(rhoPimpleFoamNumerics);
addToOpenFOAMCaseElementFactoryTable(rhoPimpleFoamNumerics);

rhoPimpleFoamNumerics::rhoPimpleFoamNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  isCompressible_=true;
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void rhoPimpleFoamNumerics::addIntoDictionaries(OFdicts& dictionaries) const
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
  controlDict["application"]="rhoPimpleFoam";

  CompressiblePIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["rho"]=stdSymmSolverSetup(1e-7, 0.01);
  solvers["p"]=GAMGSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["\"(e|h)\""]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0.1);

  solvers["rhoFinal"]=stdSymmSolverSetup(1e-7, 0.0);
  solvers["pFinal"]=GAMGPCGSolverSetup(1e-8, 0.0); //GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=smoothSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=smoothSolverSetup(1e-8, 0);
  solvers["\"(e|h)\"Final"]=smoothSolverSetup(1e-8, 0);
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

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

//  grad["default"]="cellLimited "+lqGradSchemeIfPossible()+" 1";
//  grad["grad(p)"]="Gauss linear";
////   grad["grad(U)"]="cellMDLimited "+bgrads+" 1";

  insertStandardGradientConfig(dictionaries);

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
    div["div(phi,e)"]="Gauss linear";
    div["div(phi,K)"]="Gauss linear";
    div["div(phiv,p)"]="Gauss linear";
    div["div(phi,nuTilda)"]="Gauss linear";
  }
  else
  {
    div["div(phi,U)"]="Gauss limitedLinearV 1";
    div["div(phi,k)"]="Gauss limitedLinear 1";
    div["div(phi,e)"]="Gauss linear";
    div["div(phi,K)"]="Gauss linear";
    div["div(phi,epsilon)"]="Gauss limitedLinear 1";
    div["div(phi,omega)"]="Gauss limitedLinear 1";
    div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
  }

  if (OFversion()>=230)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
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

ParameterSet rhoPimpleFoamNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}







defineType(steadyCompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(steadyCompressibleNumerics);

steadyCompressibleNumerics::steadyCompressibleNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  isCompressible_=true;
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void steadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.addDictionaryIfNonexistent("system/controlDict");
  controlDict["application"]="rhoSimpleFoam";
  controlDict["endTime"]=p_.endTime;

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
  solvers["rho"]=stdSymmSolverSetup(1e-7, 0.01);
  if (p_.transonic)
    {
      solvers["p"]=stdAsymmSolverSetup(1e-8, 0.01);
    }
  else
    {
      solvers["p"]=GAMGPCGSolverSetup(1e-8, 0.01);
    }
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["e"]=smoothSolverSetup(1e-8, 0.1);
  solvers["h"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;
  SIMPLE["rhoMin"]=p_.rhoMin;
  SIMPLE["rhoMax"]=p_.rhoMax;
  SIMPLE["consistent"]=p_.consistent;
  SIMPLE["transonic"]=p_.transonic;

  // ============ setup fvSchemes ================================



  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";

  insertStandardGradientConfig(dictionaries);

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

//  grad["default"]="cellLimited "+lqGradSchemeIfPossible()+" 1";
//  grad["grad(p)"]="Gauss linear";
////   grad["grad(U)"]="cellMDLimited "+bgrads+" 1";


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";
  div["default"]="none"; //pref+"Gauss upwind";

  OFDictData::dict& relax=fvSolution.subDict("relaxationFactors");
  {
    OFDictData::dict fieldRelax, eqnRelax;
    fieldRelax["p"]=0.3;
    fieldRelax["rho"]=0.01;
    eqnRelax["U"]=0.7;
    eqnRelax["k"]=0.7;
    eqnRelax["h"]=0.3;
    eqnRelax["e"]=0.3;
    eqnRelax["R"]=0.7;
    eqnRelax["omega"]=0.7;
    eqnRelax["epsilon"]=0.7;
    eqnRelax["nuTilda"]=0.7;
    relax["fields"]=fieldRelax;
    relax["equations"]=eqnRelax;
  }

  if (p_.setup == Parameters::setup_type::accurate)
    {
      div["div(phi,U)"]	=	pref+"Gauss linearUpwindV limitedGrad";
      div["div(phi,e)"]	=	pref+"Gauss linearUpwind limitedGrad";
      div["div(phi,h)"]	=	pref+"Gauss linearUpwind limitedGrad";
      div["div(phi,K)"]     =       pref+"Gauss linearUpwind limitedGrad";
      div["div(phid,p)"]     =       pref+"Gauss linearUpwind limitedGrad";
      div["div(phi,k)"]	=	pref+"Gauss linearUpwind grad(k)";
      div["div(phi,epsilon)"]=	pref+"Gauss linearUpwind grad(epsilon)";
      div["div(phi,omega)"]	=	pref+"Gauss linearUpwind grad(omega)";
      div["div(phi,nuTilda)"]=	pref+"Gauss linearUpwind grad(nuTilda)";
    }
  else if (p_.setup == Parameters::setup_type::stable)
    {
      div["div(phi,U)"]	=	pref+"Gauss upwind";
      div["div(phi,e)"]	=	pref+"Gauss upwind";
      div["div(phi,h)"]	=	pref+"Gauss upwind";
      div["div(phi,K)"]     =       pref+"Gauss upwind";
      div["div(phid,p)"]     =       pref+"Gauss upwind";
      div["div(phi,k)"]	=	pref+"Gauss upwind";
      div["div(phi,epsilon)"]=	pref+"Gauss upwind";
      div["div(phi,omega)"]	=	pref+"Gauss upwind";
      div["div(phi,nuTilda)"]=	pref+"Gauss upwind";
    }

  if (OFversion()>=230)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((nuEff*dev(grad(U).T())))"]="Gauss linear";
  }

  OFDictData::dict& laplacian=fvSchemes.subDict("laplacianSchemes");
  laplacian["default"]="Gauss linear localLimited UBlendingFactor 1";

  OFDictData::dict& interpolation=fvSchemes.subDict("interpolationSchemes");
  interpolation["default"]="linear";
//   interpolation["interpolate(U)"]="pointLinear";
//   interpolation["interpolate(HbyA)"]="pointLinear";

  OFDictData::dict& snGrad=fvSchemes.subDict("snGradSchemes");
  snGrad["default"]="localLimited UBlendingFactor 1";

  OFDictData::dict& fluxRequired=fvSchemes.subDict("fluxRequired");
  fluxRequired["default"]="no";
  fluxRequired["p"]="";
}

ParameterSet steadyCompressibleNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}





defineType(unsteadyCompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(unsteadyCompressibleNumerics);

unsteadyCompressibleNumerics::unsteadyCompressibleNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  isCompressible_=true;
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void unsteadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // check if LES required
  bool LES=false;
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

  bool GAMG_ok=true;
  if (OFversion()<170)
  {
   if (
       (const_cast<OpenFOAMCase&>(this->OFcase()).findElements<OverlapGGIBC>().size()>0)
       ||
       (const_cast<OpenFOAMCase&>(this->OFcase()).findElements<MixingPlaneGGIBC>().size()>0)
       )
     GAMG_ok=false;
  }
  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (OFversion()<170)
  {
    controlDict["application"]="rhoPorousMRFPimpleFoam";
  }
  else
  {
    controlDict["application"]="rhoPimpleFoam";
  }

  CompressiblePIMPLESettings pimple(p_.time_integration);
  pimple.addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers["rho"]=stdSymmSolverSetup(1e-7, 0.01);
  solvers["p"]=
      GAMG_ok ?
      GAMGSolverSetup(1e-8, 0.01) :
      stdSymmSolverSetup(1e-8, 0.01);
  solvers["U"]=smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=smoothSolverSetup(1e-8, 0.1);
  solvers["e"]=smoothSolverSetup(1e-8, 0.1);
  solvers["h"]=smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=smoothSolverSetup(1e-8, 0.1);

  double final_reltol=0.;
  if (const auto* simple = boost::get<Parameters::time_integration_type::pressure_velocity_coupling_SIMPLE_type>(&p_.time_integration.pressure_velocity_coupling))
  {
      if (simple->relax_final)
        final_reltol=0.1;
  }

  solvers["rhoFinal"]=stdSymmSolverSetup(1e-7, final_reltol);
  solvers["pFinal"]= //GAMGPCGSolverSetup(1e-8, 0.0); //GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
      GAMG_ok ?
      GAMGSolverSetup(1e-8, final_reltol*0.1) :
      stdSymmSolverSetup(1e-8, final_reltol*0.1);
  solvers["UFinal"]=smoothSolverSetup(1e-8, final_reltol);
  solvers["kFinal"]=smoothSolverSetup(1e-8, final_reltol);
  solvers["eFinal"]=smoothSolverSetup(1e-8, final_reltol);
  solvers["hFinal"]=smoothSolverSetup(1e-8, final_reltol);
  solvers["omegaFinal"]=smoothSolverSetup(1e-14, final_reltol, 1);
  solvers["epsilonFinal"]=smoothSolverSetup(1e-8, final_reltol);
  solvers["nuTildaFinal"]=smoothSolverSetup(1e-8, final_reltol);


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

//  OFDictData::dict& grad=fvSchemes.subDict("gradSchemes");

  insertStandardGradientConfig(dictionaries);

  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;
  div["default"]="none";

  div["div(phiU,p)"]="Gauss linear";
  div["div(phiv,p)"]="Gauss linear";
  div["div(phi,K)"]="Gauss linear";

  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
    div["div(phi,U)"]="Gauss linear";
    div["div(phi,k)"]="Gauss linear";
    div["div(phi,e)"]="Gauss linear";
    div["div(phi,h)"]="Gauss linear";
    div["div(phi,K)"]="Gauss linear";
    div["div(phi,nuTilda)"]="Gauss linear";
  }
  else
  {

    auto gradNameOrScheme = [&](const std::string& key) -> std::string {
      if (OFversion()>=220)
        return key;
      else
        return fvSchemes.subDict("gradSchemes").getString(key);
    };

    switch (p_.setup)
    {
      case Parameters::setup_type::accurate:
        div["div(phi,U)"]="Gauss limitedLinearV 1";
        div["div(phi,k)"]="Gauss limitedLinear 1";
        div["div(phi,h)"]="Gauss limitedLinear 1";
        div["div(phi,e)"]="Gauss limitedLinear 1";
        div["div(phi,epsilon)"]="Gauss limitedLinear 1";
        div["div(phi,omega)"]="Gauss limitedLinear 1";
        div["div(phi,nuTilda)"]="Gauss limitedLinear 1";
        break;
      case Parameters::setup_type::medium:
        div["div(phi,U)"]="Gauss linearUpwindV "+gradNameOrScheme("limitedGrad");
        div["div(phi,k)"]="Gauss linearUpwind "+gradNameOrScheme("grad(k)");
        div["div(phi,h)"]="Gauss linearUpwind "+gradNameOrScheme("limitedGrad");
        div["div(phi,e)"]="Gauss linearUpwind "+gradNameOrScheme("limitedGrad");
        div["div(phi,epsilon)"]="Gauss linearUpwind "+gradNameOrScheme("grad(epsilon)");
        div["div(phi,omega)"]="Gauss linearUpwind "+gradNameOrScheme("grad(omega)");
        div["div(phi,nuTilda)"]="Gauss linearUpwind "+gradNameOrScheme("grad(nuTilda)");
        break;
      case Parameters::setup_type::stable:
        div["div(phi,U)"]="Gauss upwind";
        div["div(phi,k)"]="Gauss upwind";
        div["div(phi,h)"]="Gauss upwind";
        div["div(phi,e)"]="Gauss upwind";
        div["div(phi,epsilon)"]="Gauss upwind";
        div["div(phi,omega)"]="Gauss upwind";
        div["div(phi,nuTilda)"]="Gauss upwind";
        break;
    }
  }

  if (OFversion()>=230)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else if (OFversion()>=210)
  {
    div["div(((rho*nuEff)*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((muEff*dev2(grad(U).T())))"]="Gauss linear";
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

ParameterSet unsteadyCompressibleNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}


}
