/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
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

class multispeciesThermodynamics
{
public:
  typedef std::map<std::string, double> SpeciesList;
  
protected:
  SpeciesList defaultComposition_;
  
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
