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









defineType(steadyCompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(steadyCompressibleNumerics);

steadyCompressibleNumerics::steadyCompressibleNumerics(OpenFOAMCase& c, const ParameterSet& ps)
: FVNumerics(c, ps, "p"),
  p_(ps)
{
  OFcase().addField("p", FieldInfo(scalarField, 	dimPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void steadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");

  setApplicationName(dictionaries, "rhoSimpleFoam");

  controlDict["endTime"]=p_.endTime;


  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  solvers["rho"]=OFcase().stdSymmSolverSetup(1e-7, 0.01);

  if (p_.transonic)
    {
      solvers["p"]=OFcase().stdAsymmSolverSetup(1e-8, 0.01);
    }
  else
    {
      solvers["p"]=
          isGAMGOk()
           ? OFcase().GAMGPCGSolverSetup(1e-8, 0.01)
           : OFcase().stdSymmSolverSetup(1e-8, 0.01);
    }

  solvers["U"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["e"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["h"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().smoothSolverSetup(1e-8, 0.1);

  OFDictData::dict& SIMPLE=fvSolution.subDict("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=p_.nNonOrthogonalCorrectors;
  SIMPLE["rhoMin"]=p_.rhoMin;
  SIMPLE["rhoMax"]=p_.rhoMax;
  SIMPLE["consistent"]=p_.consistent;
  SIMPLE["transonic"]=p_.transonic;


  setRelaxationFactors
  (
    dictionaries,
    {
      {"U",       0.7},
      {"k",       0.7},
      {"R",       0.7},
      {"omega",   0.7},
      {"epsilon", 0.7},
      {"nuTilda", 0.7},
      {"e",       0.3},
      {"h",       0.3}
    },
    {
      {pName_,       0.3},
      {"rho",       0.01}
    }
  );

  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  ddt["default"]="steadyState";


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";

  if (p_.setup == Parameters::setup_type::accurate)
    {
      div["div(phi,U)"]       =	pref+"Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
      div["div(phi,e)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(e)");
      div["div(phi,h)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(h)");
      div["div(phi,K)"]       = pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(K)");
      div["div(phid,p)"]      = pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(p)");
      div["div(phi,k)"]       =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
      div["div(phi,epsilon)"] =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
      div["div(phi,omega)"]   =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
      div["div(phi,nuTilda)"] =	pref+"Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
    }
  else if (p_.setup == Parameters::setup_type::stable)
    {
      div["div(phi,U)"]       =	pref+"Gauss upwind";
      div["div(phi,e)"]       =	pref+"Gauss upwind";
      div["div(phi,h)"]       =	pref+"Gauss upwind";
      div["div(phi,K)"]       = pref+"Gauss upwind";
      div["div(phid,p)"]      = pref+"Gauss upwind";
      div["div(phi,k)"]       =	pref+"Gauss upwind";
      div["div(phi,epsilon)"] =	pref+"Gauss upwind";
      div["div(phi,omega)"]   =	pref+"Gauss upwind";
      div["div(phi,nuTilda)"] =	pref+"Gauss upwind";
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

}

bool steadyCompressibleNumerics::isCompressible() const
{
  return true;
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
  OFcase().addField(pName_, FieldInfo(scalarField, 	dimPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
  OFcase().addField("T", FieldInfo(scalarField, 	dimTemperature, 	FieldValue({p_.Tinternal}), volField ) );
  OFcase().addField("alphat", FieldInfo(scalarField, 	dimDynViscosity, 	FieldValue({0.0}), volField ) );
}



void unsteadyCompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  bool LES=isLES();

  // ============ setup controlDict ================================

  OFDictData::dict& controlDict=dictionaries.lookupDict("system/controlDict");
  if (p_.formulation == Parameters::rhoPimpleFoam)
  {
    if (OFversion()<170)
    {
      if (OFcase().findElements<MRFZone>().size()>0)
        controlDict["application"]="rhoPorousMRFPimpleFoam";
      else
        controlDict["application"]="rhoPimpleFoam";
    }
    else
    {
      controlDict["application"]="rhoPimpleFoam";
    }
  }
  else if (p_.formulation == Parameters::sonicFoam)
  {
    if (OFversion()<170 && (OFcase().findElements<MRFZone>().size()>0) )
    {
      controlDict["application"]="sonicMRFFoam";
    }
    else
    {
      controlDict["application"]="sonicFoam";
    }
  }


  CompressiblePIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================
  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");

  double final_reltol_multiplier=0.0;
  if (const auto* simple = boost::get<Parameters::time_integration_type::pressure_velocity_coupling_SIMPLE_type>(&p_.time_integration.pressure_velocity_coupling))
  {
      if (simple->relax_final)
        final_reltol_multiplier=1.0;
  }

  for ( auto s: std::map<std::string,double>({ {"", 1.0}, {"Final", final_reltol_multiplier} }) )
  {
    if (p_.formulation == Parameters::sonicFoam)
    {
      solvers["rho"+s.first]=OFcase().diagonalSolverSetup();
    }
    else
    {
      solvers["rho"+s.first]=OFcase().stdSymmSolverSetup(1e-7, 0.1*s.second);
    }

    if (p_.time_integration.transonic || (p_.formulation == Parameters::sonicFoam) )
    {
      solvers["p"+s.first]=OFcase().stdAsymmSolverSetup(1e-8, 0.01*s.second);
    }
    else
    {
      solvers["p"+s.first]=
          isGAMGOk() ?
          OFcase().GAMGSolverSetup(1e-8, 0.01*s.second) :
          OFcase().stdSymmSolverSetup(1e-8, 0.01*s.second);
    }
    solvers["U"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["k"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["e"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["h"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["omega"+s.first]=OFcase().smoothSolverSetup(1e-12, 0.1*s.second, 1);
    solvers["epsilon"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
    solvers["nuTilda"+s.first]=OFcase().smoothSolverSetup(1e-8, 0.1*s.second);
  }

  // ============ setup fvSchemes ================================
  OFDictData::dict& fvSchemes=dictionaries.lookupDict("system/fvSchemes");

  OFDictData::dict& ddt=fvSchemes.subDict("ddtSchemes");
  if (LES)
  {
//     ddt["default"]="CrankNicolson 0.75"; // problems with pressureGradientSource (oscillations), if coefficient is =1!
    ddt["default"]          = "backward"; // channel with Retau=180 gets laminar with CrankNicholson...
  }
  else
  {
    ddt["default"]          = "Euler";
  }


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string suf;

  div["div(phiU,p)"]        = "Gauss limitedLinear 1";
  div["div(phiv,p)"]        = "Gauss limitedLinear 1";
  div["div(phid,p)"]        = "Gauss limitedLinear 1";
  div["div(phi,K)"]         = "Gauss limitedLinear 1";


  if (LES)
  {
    /*if (OFversion()>=220)
      div["div(phi,U)"]="Gauss LUST grad(U)";
    else*/
    div["div(phi,U)"]       = "Gauss linear";
    div["div(phi,k)"]       = "Gauss linear";
    div["div(phi,e)"]       = "Gauss linear";
    div["div(phi,h)"]       = "Gauss linear";
    div["div(phi,K)"]       = "Gauss linear";
    div["div(phi,nuTilda)"] = "Gauss linear";
  }
  else
  {

    switch (p_.setup)
    {
      case Parameters::setup_type::accurate:
        div["div(phi,U)"]       = "Gauss limitedLinearV 1";
        div["div(phi,k)"]       = "Gauss limitedLinear 1";
        div["div(phi,h)"]       = "Gauss limitedLinear 1";
        div["div(phi,e)"]       = "Gauss limitedLinear 1";
        div["div(phi,epsilon)"] = "Gauss limitedLinear 1";
        div["div(phi,omega)"]   = "Gauss limitedLinear 1";
        div["div(phi,nuTilda)"] = "Gauss limitedLinear 1";
        break;
      case Parameters::setup_type::medium:
        div["div(phi,U)"]       = "Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
        div["div(phi,k)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
        div["div(phi,h)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(h)");
        div["div(phi,e)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(e)");
        div["div(phi,epsilon)"] = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
        div["div(phi,omega)"]   = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
        div["div(phi,nuTilda)"] = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
        break;
      case Parameters::setup_type::stable:
        div["div(phi,U)"]       = "Gauss upwind";
        div["div(phi,k)"]       = "Gauss upwind";
        div["div(phi,h)"]       = "Gauss upwind";
        div["div(phi,e)"]       = "Gauss upwind";
        div["div(phi,epsilon)"] = "Gauss upwind";
        div["div(phi,omega)"]   = "Gauss upwind";
        div["div(phi,nuTilda)"] = "Gauss upwind";
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
  else if (OFversion()>=164)
  {
    div["div((muEff*dev2(T(grad(U)))))"]="Gauss linear";
  }
  else
  {
    div["div((muEff*dev2(grad(U).T())))"]="Gauss linear";
  }

}


bool unsteadyCompressibleNumerics::isCompressible() const
{
  return true;
}


ParameterSet unsteadyCompressibleNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}


}
