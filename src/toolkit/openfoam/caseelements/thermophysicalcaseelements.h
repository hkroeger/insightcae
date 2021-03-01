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

#include "openfoam/caseelements/basic/thermodynamicmodel.h"

#include <vector>
#include <string>
#include <array>
#include <map>

namespace insight {



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






double sutherland_As(double mu, double Ts);


class SpeciesData
{

  static void modifyDefaults(ParameterSet& ps);

public:

  struct ElementData {
    int id;
    std::string name;
    double M;
  };

  typedef std::vector<std::pair<double, SpeciesData> > SpeciesMixture;

  // from https://www.qmul.ac.uk/sbcs/iupac/AtWt/
  static const std::map<std::string, SpeciesData::ElementData> elements;


#include "thermophysicalcaseelements__SpeciesData__Parameters.h"

/*
PARAMETERSET>>> SpeciesData Parameters
addTo_makeDefault { modifyDefaults(p); }

name = string "N2" "Name of specie (i.e. the field name). Has to be unique!" *necessary

properties = selectablesubset {{

  fromLibrary set {
   specie = selection ( no_library_available ) no_library_available "Species library entry"
  }

  custom set {

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
       mu = double 1.8e-5 "Dynamic viscosity"
       Pr = double 0.7 "Prandtl number"
     }

     sutherland
     set {
       mu = double 9.41e-6 "Dynamic viscosity at $T_{ref}$"
       Tref = double 440 "Reference temperature $T_{ref}$"
       Pr = double 0.7 "Prandtl number"
     }

    }} constant "Transport properties"



    elements = array [ set {
     element = string "N" "Name of the element"
     number = double 2 "Number of atoms per molecule. May be a fraction, if the specie represents a mixture."
     } ] *1 "Elemental composition"

   }

}} fromLibrary "Data of the specie."

<<<PARAMETERSET
*/

protected:
//  Parameters p_;
  std::string name_;
  Parameters::properties_custom_type p_;

  static struct SpeciesLibrary
      : public std::map<std::string, Parameters::properties_custom_type>
  {
    declareFactoryTable(SpeciesLibrary, LIST(const ParameterSet& ps = ParameterSet() ), LIST(ps));

    SpeciesLibrary();
  } speciesLibrary_;


public:
  SpeciesData(const ParameterSet& ps);
  SpeciesData(
      const std::string& name,
      SpeciesMixture mixture
      );

  double M() const;

  std::string transportType() const;
  std::string thermoType() const;

  void insertSpecieEntries(OFDictData::dict& d) const;
  void insertThermodynamicsEntries(OFDictData::dict& d) const;
  void insertTransportEntries(OFDictData::dict& d) const;
  void insertElementsEntries(OFDictData::dict& d) const;

  std::pair<double,double> temperatureLimits() const;
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
   mu = double 1.8e-5 "Dynamic viscosity"
   Pr = double 0.7 "Prandtl number"
 }

 sutherland
 set {
   mu = double 9.41e-6 "Dynamic viscosity at $T_{ref}$"
   Tref = double 440 "Reference temperature $T_{ref}$"
   Pr = double 0.7 "Prandtl number"
 }

}} constant "Transport properties"




equationOfState = selectablesubset {{

 rhoConst
 set {
   rho = double 1.0 "[kg/m^3] Density"
 }

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

    static std::string category() { return "Material Properties"; }
};






class compressibleMixtureThermophysicalProperties
: public thermodynamicModel
{

public:
  typedef std::map<std::string, boost::variant<boost::blank,SpeciesData> > SpeciesList;
  typedef std::map<std::string, double> SpeciesMassFractionList;

  std::vector<std::string> speciesNames() const;
  SpeciesList species() const;
  SpeciesMassFractionList defaultComposition() const;

  void removeSpecie(const std::string& name);
  void addSpecie(const std::string& name, SpeciesData d);

protected:
  std::string getTransportType() const;
  std::string getThermoType() const;

public:
#include "thermophysicalcaseelements__compressibleMixtureThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleMixtureThermophysicalProperties Parameters

inertSpecie = string "N2" ""

composition = selectablesubset {{

  fromFile set {
    foamChemistryThermoFile = path "" "" *necessary
  }

  fromList set {
    species = array [
     includedset "insight::SpeciesData::Parameters"
    ] *1 "Species in the mixture"
  }

 }} fromList ""



reactions = selectablesubset {{

 none set {
 }

 fromFile set {
    foamChemistryFile = path "" "" *necessary
 }

}} none ""



combustionModel = selectablesubset {{
 none set { }
 EDC set { }
 PaSR set {
  Cmix = double 1.0 ""
 }
}} EDC "The combustion model"

<<<PARAMETERSET
*/


protected:
  Parameters p_;
  SpeciesList species_;

public:
  compressibleMixtureThermophysicalProperties(OpenFOAMCase& c, ParameterSet const& p = Parameters::makeDefault());
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

};


}

#endif // INSIGHT_THERMOPHYSICALCASEELEMENTS_H
