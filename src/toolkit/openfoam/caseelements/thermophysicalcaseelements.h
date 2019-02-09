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

#ifndef INSIGHT_THERMOPHYSICALCASEELEMENTS_H
#define INSIGHT_THERMOPHYSICALCASEELEMENTS_H

#include "basiccaseelements.h"
#include "base/resultset.h"

namespace insight {




class thermodynamicModel
: public OpenFOAMCaseElement
{
public:
  thermodynamicModel(OpenFOAMCase& c);
};




class cavitatingFoamThermodynamics
: public thermodynamicModel
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    (psiv, double, 2.5e-6)
    (psil, double, 5e-7)
    (pSat, double, 2000.0)
    (rholSat, double, 1025.0)
    (rhoMin, double, 0.001)
  )

protected:
  Parameters p_;
  
public:
  cavitatingFoamThermodynamics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};




class perfectGasSinglePhaseThermophysicalProperties
    : public thermodynamicModel
{

public:
#include "thermophysicalcaseelements__perfectGasSinglePhaseThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> perfectGasSinglePhaseThermophysicalProperties Parameters

Tref = double 300 "Reference temperature $T_{ref}$"
pref = double 1e5 "Reference pressure $p_{ref}$"

rho = double 1.0 "Density at $T_{ref}$ and $p_{ref}$"
nu = double 1.8e-5 "Kinematic viscosity at $T_{ref}$"
kappa = double 1.4 "Heat capacity reatio"
Pr = double 0.7 "Prandtl number"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "perfectGasSinglePhaseThermophysicalProperties" );
    perfectGasSinglePhaseThermophysicalProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Material Properties"; }
};




class multispeciesThermodynamics
{
public:
  typedef std::map<std::string, double> SpeciesList;
  
protected:
  SpeciesList defaultComposition_;

public:
  inline const SpeciesList& defaultComposition() const { return defaultComposition_; }
};

class detailedGasReactionThermodynamics
: public thermodynamicModel,
  public multispeciesThermodynamics
{
public:
  CPPX_DEFINE_OPTIONCLASS(Parameters, CPPX_OPTIONS_NO_BASE,
    //(combustionModel, detailGasReactionModel::Ptr, "")
    (inertSpecie, std::string, "N2")
    (foamChemistryFile, boost::filesystem::path, "")
    (foamChemistryThermoFile, boost::filesystem::path, "")
    (Cmix, double, 1.0)
  )

protected:
  Parameters p_;
  
public:
  detailedGasReactionThermodynamics(OpenFOAMCase& c, Parameters const& p = Parameters());
  virtual void addIntoDictionaries(OFdicts& dictionaries) const;
};


}

#endif // INSIGHT_THERMOPHYSICALCASEELEMENTS_H
