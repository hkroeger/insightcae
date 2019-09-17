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
  thermodynamicModel(OpenFOAMCase& c, const ParameterSet& ps);
};




class cavitatingFoamThermodynamics
: public thermodynamicModel
{
public:
#include "thermophysicalcaseelements__cavitatingFoamThermodynamics__Parameters.h"
/*
PARAMETERSET>>> cavitatingFoamThermodynamics Parameters

psiv = double 2.5e-6 ""
psil = double 5e-7 ""
pSat = double 2000.0 "Cavitation pressure"
rholSat = double 1025.0 "Liquid density"
rhoMin = double 0.001 "Lower threshold for density"

<<<PARAMETERSET
*/

protected:
  Parameters p_;
  
public:
  cavitatingFoamThermodynamics(OpenFOAMCase& c, ParameterSet const& p = Parameters::makeDefault());
  void addIntoDictionaries(OFdicts& dictionaries) const override;
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
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static ParameterSet defaultParameters()
    {
        return Parameters::makeDefault();
    }
    static std::string category() { return "Material Properties"; }
};




class compressibleSinglePhaseThermophysicalProperties
    : public thermodynamicModel
{

public:
#include "thermophysicalcaseelements__compressibleSinglePhaseThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleSinglePhaseThermophysicalProperties Parameters

M = double 28.9 "[kg/kmol] molar weight"

thermo = selectablesubset {{

 constant
 set {
   Cp = double 1007 "[J/kg/K] Heat capacity"
   Hf = double 0. "[J/kg] Enthalpy of formation"
 }

 janaf
 set {
  Tlow = double 100 "[T] Lower temperature bound of approximation"
  Thi = double 6000 "[T] Upper temperature bound of approximation"
  Tmid = double 1000 "[T] Switching temperature between hi and low polynomial"

  coeffs_lo = vector (3.5309628 -0.0001236595 -5.0299339e-07 2.4352768e-09 -1.4087954e-12 -1046.9637 2.9674391) "Lower temperature polynomial coefficients"
  coeffs_hi = vector (2.9525407 0.0013968838 -4.9262577e-07 7.8600091e-11 -4.6074978e-15 -923.93753 5.8718221) "Higher temperature polynomial coefficients"
 }

}} constant "Thermodynamics properties"




transport = selectablesubset {{

 constant
 set {
   nu = double 1.8e-5 "Kinematic viscosity"
   Pr = double 0.7 "Prandtl number"
 }

 sutherland
 set {
   nu = double 9.41e-6 "Kinematic viscosity at $T_{ref}$"
   Tref = double 440 "Reference temperature $T_{ref}$"
 }

}} constant "Transport properties"




equationOfState = selectablesubset {{

 idealGas
 set { }

 PengRobinson
 set {
  Tc = double 617. "[K] Critical temperature"
  Pc = double 3622400.0 "[Pa] Critical pressure"
  omega = double 0.304 "Acentric factor"
 }

}} idealGas "Equation of state"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "compressibleSinglePhaseThermophysicalProperties" );
    compressibleSinglePhaseThermophysicalProperties ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );

    std::string requiredThermoType() const;
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

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
#include "thermophysicalcaseelements__detailedGasReactionThermodynamics__Parameters.h"
/*
PARAMETERSET>>> detailedGasReactionThermodynamics Parameters

inertSpecie = string "N2" ""
foamChemistryFile = path "" ""
foamChemistryThermoFile = path "" ""
Cmix = double 1.0 ""

<<<PARAMETERSET
*/


protected:
  Parameters p_;
  
public:
  detailedGasReactionThermodynamics(OpenFOAMCase& c, ParameterSet const& p = Parameters::makeDefault());
  void addIntoDictionaries(OFdicts& dictionaries) const override;
};


}

#endif // INSIGHT_THERMOPHYSICALCASEELEMENTS_H
