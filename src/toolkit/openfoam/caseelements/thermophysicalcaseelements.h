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
#include "openfoam/caseelements/thermodynamics/speciesdata.h"

#include <vector>
#include <string>
#include <array>
#include <map>

#include "thermophysicalcaseelements__cavitatingFoamThermodynamics__Parameters_headers.h"

namespace insight {



class cavitatingFoamThermodynamics
: public thermodynamicModel
{
public:
#include "thermophysicalcaseelements__cavitatingFoamThermodynamics__Parameters.h"
/*
PARAMETERSET>>> cavitatingFoamThermodynamics Parameters
inherits thermodynamicModel::Parameters

psiv = double 2.5e-6 ""
psil = double 5e-7 ""
pSat = double 2000.0 "Cavitation pressure"
rholSat = double 1025.0 "Liquid density"
rhoMin = double 0.001 "Lower threshold for density"

createGetters
<<<PARAMETERSET
*/
  
public:
  cavitatingFoamThermodynamics(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addIntoDictionaries(OFdicts& dictionaries) const override;
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
  void addSpecie(const std::string& name, const SpeciesData& d);

protected:
  std::string getTransportType() const;
  std::string getThermoType() const;

public:
#include "thermophysicalcaseelements__compressibleMixtureThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleMixtureThermophysicalProperties Parameters
inherits thermodynamicModel::Parameters

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

createGetters
<<<PARAMETERSET
*/


protected:
  SpeciesList species_;

public:
  compressibleMixtureThermophysicalProperties(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

};


}

#endif // INSIGHT_THERMOPHYSICALCASEELEMENTS_H
