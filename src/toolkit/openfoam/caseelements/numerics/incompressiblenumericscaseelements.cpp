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




defineType(steadyIncompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(steadyIncompressibleNumerics);


steadyIncompressibleNumerics::steadyIncompressibleNumerics(OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName)
: FVNumerics(c, ps, pName),
  p_(ps)
{
  OFcase().addField(pName, FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
}


void steadyIncompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // ============ setup controlDict ================================
  setApplicationName(dictionaries, "simpleFoam");


  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");

  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers[pName_]        =
      isGAMGOk() ?
        OFcase().GAMGPCGSolverSetup(1e-7, 0.01)
      :
        OFcase().stdSymmSolverSetup(1e-7, 0.01)
        ;
  solvers["U"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["R"]        = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]    = OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]  = OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]  = OFcase().smoothSolverSetup(1e-8, 0.1);

  setRelaxationFactors
  (
    dictionaries,
    {
      {"U",       0.7},
      {"k",       0.7},
      {"R",       0.7},
      {"omega",   0.7},
      {"epsilon", 0.7},
      {"nuTilda", 0.7}
    },
    {
      {pName_,       0.3}
    }
  );

  OFDictData::dict& SIMPLE=fvSolution.addSubDictIfNonexistent("SIMPLE");
  SIMPLE["nNonOrthogonalCorrectors"]=0;
  SIMPLE["pRefCell"]=0;
  SIMPLE["pRefValue"]=p_.pinternal;

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


  OFDictData::dict& div=fvSchemes.subDict("divSchemes");
  std::string pref, suf;
  if (OFversion()>=220) pref="bounded ";

  div["default"]="none";

  div["div(phi,U)"]	=	pref+"Gauss linearUpwindV "+gradNameOrScheme(dictionaries, "grad(U)");
  div["div(phi,nuTilda)"]       = "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(nuTilda)");
  div["div(phi,k)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(k)");
  div["div(phi,epsilon)"]	= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(epsilon)");
  div["div(phi,omega)"]		= "Gauss linearUpwind "+gradNameOrScheme(dictionaries, "grad(omega)");
  div["div(phi,R)"]             = "Gauss upwind";
  div["div(R)"]                 = "Gauss linear";

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

}


bool steadyIncompressibleNumerics::isCompressible() const
{
  return false;
}

ParameterSet steadyIncompressibleNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}








defineType(unsteadyIncompressibleNumerics);
addToOpenFOAMCaseElementFactoryTable(unsteadyIncompressibleNumerics);

unsteadyIncompressibleNumerics::unsteadyIncompressibleNumerics(OpenFOAMCase& c, const ParameterSet& ps, const std::string& pName)
: FVNumerics(c, ps, pName),
  p_(ps)
{
  OFcase().addField(pName_, FieldInfo(scalarField, 	dimKinPressure, 	FieldValue({p_.pinternal}), volField ) );
  OFcase().addField("U", FieldInfo(vectorField, 	dimVelocity, 		std::vector<double>(p_.Uinternal.begin(), p_.Uinternal.end()), volField ) );
}



void unsteadyIncompressibleNumerics::addIntoDictionaries(OFdicts& dictionaries) const
{
  FVNumerics::addIntoDictionaries(dictionaries);

  // check if LES required
  bool LES=isLES();

  // ============ setup controlDict ================================

  if (OFcase().findElements<dynamicMesh>().size()>0)
   setApplicationName(dictionaries, "pimpleDyMFoam");
  else
   setApplicationName(dictionaries, "pimpleFoam");

  PIMPLESettings(p_.time_integration).addIntoDictionaries(OFcase(), dictionaries);

  // ============ setup fvSolution ================================

  OFDictData::dict& fvSolution=dictionaries.lookupDict("system/fvSolution");
  OFDictData::dict& solvers=fvSolution.subDict("solvers");
  solvers[pName_]=isGAMGOk() ? OFcase().GAMGSolverSetup(1e-8, 0.01) : OFcase().stdSymmSolverSetup(1e-8, 0.01); //stdSymmSolverSetup(1e-7, 0.01);
  solvers["U"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["k"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["omega"]=OFcase().smoothSolverSetup(1e-12, 0.1, 1);
  solvers["epsilon"]=OFcase().smoothSolverSetup(1e-8, 0.1);
  solvers["nuTilda"]=OFcase().smoothSolverSetup(1e-8, 0.1);

  solvers[pName_+"Final"]=isGAMGOk() ? OFcase().GAMGPCGSolverSetup(1e-8, 0.0) : OFcase().stdSymmSolverSetup(1e-8, 0.0); //GAMGSolverSetup(1e-8, 0.0); //stdSymmSolverSetup(1e-7, 0.0);
  solvers["UFinal"]=OFcase().smoothSolverSetup(1e-8, 0.0);
  solvers["kFinal"]=OFcase().smoothSolverSetup(1e-8, 0);
  solvers["omegaFinal"]=OFcase().smoothSolverSetup(1e-14, 0, 1);
  solvers["epsilonFinal"]=OFcase().smoothSolverSetup(1e-8, 0);
  solvers["nuTildaFinal"]=OFcase().smoothSolverSetup(1e-8, 0);


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

}


bool unsteadyIncompressibleNumerics::isCompressible() const
{
  return false;
}

ParameterSet unsteadyIncompressibleNumerics::defaultParameters()
{
    return Parameters::makeDefault();
}


}
