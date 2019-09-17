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

#include "thermophysicalcaseelements.h"
#include "openfoam/caseelements/numerics/numericscaseelements.h"
#include "openfoam/caseelements/numerics/compressiblenumericscaseelements.h"
#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include <utility>
#include "boost/assign.hpp"
#include "boost/lexical_cast.hpp"

#include "gnuplot-iostream.h"

using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;


namespace insight
{

  
thermodynamicModel::thermodynamicModel(OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "thermodynamicModel", ps)
{
}

cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, const ParameterSet& ps)
: thermodynamicModel(c, ps),
  p_(ps)
{
}

void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.addDictionaryIfNonexistent("constant/thermodynamicProperties");
  thermodynamicProperties["barotropicCompressibilityModel"]="linear";
  thermodynamicProperties["psiv"]=OFDictData::dimensionedData("psiv", 
							      OFDictData::dimension(0, -2, 2), 
                                                              p_.psiv);
  thermodynamicProperties["rholSat"]=OFDictData::dimensionedData("rholSat", 
								 OFDictData::dimension(1, -3), 
                                                                 p_.rholSat);
  thermodynamicProperties["psil"]=OFDictData::dimensionedData("psil", 
								 OFDictData::dimension(0, -2, 2), 
                                                                 p_.psil);
  thermodynamicProperties["pSat"]=OFDictData::dimensionedData("pSat", 
								 OFDictData::dimension(1, -1, -2), 
                                                                 p_.pSat);
  thermodynamicProperties["rhoMin"]=OFDictData::dimensionedData("rhoMin", 
								 OFDictData::dimension(1, -3), 
                                                                 p_.rhoMin);
}




defineType(perfectGasSinglePhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(perfectGasSinglePhaseThermophysicalProperties);

perfectGasSinglePhaseThermophysicalProperties::perfectGasSinglePhaseThermophysicalProperties( OpenFOAMCase& c, const ParameterSet& ps )
: thermodynamicModel(c, ps),
  p_(ps)
{
}

void perfectGasSinglePhaseThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermophysicalProperties=dictionaries.addDictionaryIfNonexistent("constant/thermophysicalProperties");

  enum thermoType { hePsiThermo, heRhoThermo } tht = hePsiThermo;
  try
  {
    const FVNumerics* nce = OFcase().get<FVNumerics>("FVNumerics");

    if (
        dynamic_cast<const buoyantSimpleFoamNumerics*>(nce) ||
        dynamic_cast<const buoyantPimpleFoamNumerics*>(nce) ||
        dynamic_cast<const steadyCompressibleNumerics*>(nce)
        )
      {
        tht=heRhoThermo;
      }
  }
  catch (...)
  {
    insight::Warning("Cannot determine required thermo type! Using default hePsiThermo.");
  }

  OFDictData::dict tt;

  if (tht==hePsiThermo)
    {
      tt["type"]="hePsiThermo";
      tt["thermo"]="eConst";
      tt["energy"]="sensibleInternalEnergy";
    }
  else if (tht==heRhoThermo)
    {
      tt["type"]="heRhoThermo";
      tt["thermo"]="hConst";
      tt["energy"]="sensibleEnthalpy";
    }

  tt["mixture"]="pureMixture";
  tt["transport"]="const";
  tt["equationOfState"]="perfectGas";
  tt["specie"]="specie";

  thermophysicalProperties["thermoType"]=tt;

  const double R=8.3144598;
  double M = p_.rho*R*p_.Tref/p_.pref;

  OFDictData::dict mixture, mix_sp, mix_td, mix_tr;
  mix_sp["nMoles"]=1.;
  mix_sp["molWeight"]=1e3*M;

  if (tht==hePsiThermo)
    {
      double cv = R/(p_.kappa-1.) / M;
      mix_td["Cv"]=cv;
    }
  else if (tht==heRhoThermo)
    {
      double cp = p_.kappa*R/(p_.kappa-1.) / M;
      mix_td["Cp"]=cp;
    }

  mix_td["Hf"]=0.;

  mix_tr["mu"]=p_.nu*p_.rho;
  mix_tr["Pr"]=p_.Pr;

  mixture["specie"]=mix_sp;
  mixture["thermodynamics"]=mix_td;
  mixture["transport"]=mix_tr;

  thermophysicalProperties["mixture"]=mixture;
}




defineType(compressibleSinglePhaseThermophysicalProperties);
addToOpenFOAMCaseElementFactoryTable(compressibleSinglePhaseThermophysicalProperties);

compressibleSinglePhaseThermophysicalProperties::compressibleSinglePhaseThermophysicalProperties( OpenFOAMCase& c, const ParameterSet& ps )
: thermodynamicModel(c, ps),
  p_(ps)
{
}

std::string compressibleSinglePhaseThermophysicalProperties::requiredThermoType() const
{
  std::string tt = "hPsiThermo";

  const FVNumerics* nce = OFcase().get<FVNumerics>("FVNumerics");

  if (
      dynamic_cast<const buoyantSimpleFoamNumerics*>(nce) ||
      dynamic_cast<const buoyantPimpleFoamNumerics*>(nce)
      )
  {
    if (OFversion()<170)
    {
      tt="hPsiThermo";
    }
    else
    {
      tt="heRhoThermo";
    }
  }
  else if (dynamic_cast<const steadyCompressibleNumerics*>(nce))
  {
    if (OFversion()<170)
    {
      tt="hPsiThermo";
    }
    else
    {
      tt="heRhoThermo";
    }
  }
  else if (const auto t = dynamic_cast<const unsteadyCompressibleNumerics*>(nce) )
  {
    if (OFversion()<170)
    {
      if (unsteadyCompressibleNumerics::Parameters(t->parameters()).formulation == unsteadyCompressibleNumerics::Parameters::sonicFoam)
      {
        tt="ePsiThermo";
      }
      else // rhoPimpleFoam
      {
        tt="hPsiThermo";
      }
    }
    else
    {
      tt="hePsiThermo";
    }
  }

  return tt;
}

void compressibleSinglePhaseThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{

  OFDictData::dict& thermophysicalProperties =
      dictionaries.addDictionaryIfNonexistent("constant/thermophysicalProperties");

  std::string tt = requiredThermoType();

  if (OFversion()<170)
  {
    tt += "<pureMixture<";

    std::string mixp_eqn, mixp_thermo, mixp_transp, mixp =
        boost::str(boost::format("specie 1 %g") % p_.M);

    if (const auto *ct = boost::get<Parameters::transport_constant_type>(&p_.transport))
    {
      tt+="constTransport";
      mixp_transp =
          boost::str(boost::format("%g %g") % ct->nu % ct->Pr );
    }
    else if (const auto *st = boost::get<Parameters::transport_sutherland_type>(&p_.transport))
    {
      tt+="sutherlandTransport";
      mixp_transp =
          boost::str(boost::format("%g %g") % st->nu % st->Tref );
    }

    tt+="<specieThermo<";

    if (const auto *ct = boost::get<Parameters::thermo_constant_type>(&p_.thermo))
    {
      tt+="hConstThermo";
      mixp_thermo =
          boost::str(boost::format("%g %g") % ct->Cp % ct->Hf );
    }
    else if (const auto *jt = boost::get<Parameters::thermo_janaf_type>(&p_.thermo))
    {
      tt+="janafThermo";
      mixp_thermo =
          boost::str(boost::format("%g %g %g\n") % jt->Tlow % jt->Thi % jt->Tmid );

      for (arma::uword i=0; i<jt->coeffs_hi.size(); i++)
      {
        mixp_thermo += " "+boost::lexical_cast<std::string>(jt->coeffs_hi(i));
      }
      mixp_thermo+="\n";

      for (arma::uword i=0; i<jt->coeffs_lo.size(); i++)
      {
        mixp_thermo += " "+boost::lexical_cast<std::string>(jt->coeffs_lo(i));
      }
      mixp_thermo+="\n";
    }

    tt+="<";

    if (const auto *pe = boost::get<Parameters::equationOfState_idealGas_type>(&p_.equationOfState))
    {
      tt+="perfectGas";
    }
    else if (const auto *pre = boost::get<Parameters::equationOfState_PengRobinson_type>(&p_.equationOfState))
    {
      tt+="PengRobinsonGas";
      mixp_eqn =
           boost::str(boost::format("%g %g %g %g")
            % pre->Tc % 0.0 % pre->Pc % pre->omega );
    }

    tt+=">>>>>";

    thermophysicalProperties["thermoType"]=tt;
    thermophysicalProperties["mixture"]=mixp +"\n "+ mixp_eqn +"\n "+ mixp_thermo +"\n "+ mixp_transp;
  }
  else
  {
    throw insight::Exception("OF version currently unsupported.");
  }

}






detailedGasReactionThermodynamics::detailedGasReactionThermodynamics
(
  OpenFOAMCase& c, 
  const ParameterSet& ps
)
: thermodynamicModel(c, ps),
  p_(ps)
{
  std::ifstream tf(p_.foamChemistryThermoFile.c_str());
  OFDictData::dict td;
  readOpenFOAMDict(tf, td);
  
  c.addField("Ydefault", FieldInfo(scalarField, 	dimless, 	FieldValue({0.0}), volField ) );
  for (const OFDictData::dict::value_type e: td)
  {
    std::string specie = e.first;
    double v=0.0;
    if (specie==p_.inertSpecie) v=1.0;

    defaultComposition_[specie]=v;

    c.addField(specie, FieldInfo(scalarField, 	dimless, 	FieldValue({v}), volField ) );
  }
}

void detailedGasReactionThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.addDictionaryIfNonexistent("constant/thermophysicalProperties");
  
  OFDictData::dict tt;
  tt["type"]="hePsiThermo";
  tt["mixture"]="reactingMixture";  
  tt["transport"]="sutherland";
  tt["thermo"]="janaf";
  tt["energy"]="sensibleEnthalpy";
  tt["equationOfState"]="perfectGas";
  tt["specie"]="specie";
  thermodynamicProperties["thermoType"]=tt;  
  
  thermodynamicProperties["inertSpecie"]=p_.inertSpecie;
  thermodynamicProperties["chemistryReader"]="foamChemistryReader";
  thermodynamicProperties["foamChemistryFile"]="\""+p_.foamChemistryFile.string()+"\"";
  thermodynamicProperties["foamChemistryThermoFile"]="\""+p_.foamChemistryThermoFile.string()+"\"";

  
  OFDictData::dict& combustionProperties=dictionaries.addDictionaryIfNonexistent("constant/combustionProperties");
  combustionProperties["combustionModel"]="PaSR<psiChemistryCombustion>";
  combustionProperties["active"]=true;

  OFDictData::dict pd;
  pd["Cmix"]=p_.Cmix;
  pd["turbulentReaction"]=true;
  combustionProperties["PaSRCoeffs"]=pd;

  OFDictData::dict& chemistryProperties=dictionaries.addDictionaryIfNonexistent("constant/chemistryProperties");
  
  OFDictData::dict ct;
  ct["chemistrySolver"]="EulerImplicit";
  ct["chemistryThermo"]="psi";
  chemistryProperties["chemistryType"]=ct;
  
  OFDictData::dict eic;
  eic["cTauChem"]=1;
  eic["equilibriumRateLimiter"]=false;
  chemistryProperties["EulerImplicitCoeffs"]=eic;
  
  chemistryProperties["chemistry"]=true;
  chemistryProperties["initialChemicalTimeStep"]=1e-7;
}

}
