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

#include "thermophysicalcaseelements.h"
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

  
thermodynamicModel::thermodynamicModel(OpenFOAMCase& c)
: OpenFOAMCaseElement(c, "thermodynamicModel")
{
}

cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, const Parameters& p)
: thermodynamicModel(c),
  p_(p)
{
}

void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.addDictionaryIfNonexistent("constant/thermodynamicProperties");
  thermodynamicProperties["barotropicCompressibilityModel"]="linear";
  thermodynamicProperties["psiv"]=OFDictData::dimensionedData("psiv", 
							      OFDictData::dimension(0, -2, 2), 
							      p_.psiv());
  thermodynamicProperties["rholSat"]=OFDictData::dimensionedData("rholSat", 
								 OFDictData::dimension(1, -3), 
								 p_.rholSat());
  thermodynamicProperties["psil"]=OFDictData::dimensionedData("psil", 
								 OFDictData::dimension(0, -2, 2), 
								 p_.psil());
  thermodynamicProperties["pSat"]=OFDictData::dimensionedData("pSat", 
								 OFDictData::dimension(1, -1, -2), 
								 p_.pSat());
  thermodynamicProperties["rhoMin"]=OFDictData::dimensionedData("rhoMin", 
								 OFDictData::dimension(1, -3), 
								 p_.rhoMin());
}

detailedGasReactionThermodynamics::detailedGasReactionThermodynamics
(
  OpenFOAMCase& c, 
  const detailedGasReactionThermodynamics::Parameters& p
)
: thermodynamicModel(c),
  p_(p)
{
  std::ifstream tf(p_.foamChemistryThermoFile().c_str());
  OFDictData::dict td;
  readOpenFOAMDict(tf, td);
  
  c.addField("Ydefault", FieldInfo(scalarField, 	dimless, 	list_of(0.0), volField ) );
  BOOST_FOREACH(const OFDictData::dict::value_type e, td)
  {
    std::string specie = e.first;
    double v=0.0;
    if (specie==p_.inertSpecie()) v=1.0;

    defaultComposition_[specie]=v;

    c.addField(specie, FieldInfo(scalarField, 	dimless, 	list_of(v), volField ) );
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
  
  thermodynamicProperties["inertSpecie"]=p_.inertSpecie();
  thermodynamicProperties["chemistryReader"]="foamChemistryReader";
  thermodynamicProperties["foamChemistryFile"]="\""+p_.foamChemistryFile().string()+"\"";
  thermodynamicProperties["foamChemistryThermoFile"]="\""+p_.foamChemistryThermoFile().string()+"\"";

  
  OFDictData::dict& combustionProperties=dictionaries.addDictionaryIfNonexistent("constant/combustionProperties");
  combustionProperties["combustionModel"]="PaSR<psiChemistryCombustion>";
  combustionProperties["active"]=true;

  OFDictData::dict pd;
  pd["Cmix"]=p_.Cmix();
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
