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

#include "base/exception.h"
#include "base/tools.h"
#include "boost/variant/detail/apply_visitor_binary.hpp"
#include "boost/variant/static_visitor.hpp"
#include "openfoam/openfoamcase.h"

#include "openfoam/caseelements/numerics/reactingfoamnumerics.h"


#include <algorithm>
#include <utility>


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;


namespace insight
{




cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, ParameterSetInput ip)
: thermodynamicModel(c, ip.forward<Parameters>())
{}




void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.lookupDict("constant/thermodynamicProperties");
  thermodynamicProperties["barotropicCompressibilityModel"]="linear";
  thermodynamicProperties["psiv"]=OFDictData::dimensionedData("psiv", 
							      OFDictData::dimension(0, -2, 2), 
                                                              p().psiv);
  thermodynamicProperties["rholSat"]=OFDictData::dimensionedData("rholSat", 
								 OFDictData::dimension(1, -3), 
                                                                 p().rholSat);
  thermodynamicProperties["psil"]=OFDictData::dimensionedData("psil", 
								 OFDictData::dimension(0, -2, 2), 
                                                                 p().psil);
  thermodynamicProperties["pSat"]=OFDictData::dimensionedData("pSat", 
								 OFDictData::dimension(1, -1, -2), 
                                                                 p().pSat);
  thermodynamicProperties["rhoMin"]=OFDictData::dimensionedData("rhoMin", 
								 OFDictData::dimension(1, -3), 
                                                                 p().rhoMin);
}




void compressibleMixtureThermophysicalProperties::modifyDefaults(ParameterSet &ps)
{
    ps.get<LabeledArrayParameter>("composition/species")
        .changeLabel("specie0", "N2");

}




std::vector<std::string> compressibleMixtureThermophysicalProperties::speciesNames() const
{
  std::vector<std::string> sns;

  std::transform(species_.begin(), species_.end(),
                 std::back_inserter(sns),
                 [](const SpeciesList::value_type& e)
                  {
                    return e.first;
                  }
  );

//  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p().composition))
//  {
//    std::transform(list->species.begin(), list->species.end(),
//                   std::back_inserter(sns),
//                   [](const Parameters::composition_fromList_type::species_default_type& e)
//                    {
//                      return e.name;
//                    }
//    );
//  }
//  else if (const auto * file = boost::get<Parameters::composition_fromFile_type>(&p().composition))
//  {
//    std::istream& tf = (file->foamChemistryThermoFile)->stream();

//    OFDictData::dict td;
//    readOpenFOAMDict(tf, td);
//    std::transform(td.begin(), td.end(),
//                   std::back_inserter(sns),
//                   [](const OFDictData::dict::value_type& tde)
//                    {
//                      return tde.first;
//                    }
//    );
//  }

  return sns;
}



compressibleMixtureThermophysicalProperties::SpeciesList
compressibleMixtureThermophysicalProperties::species() const
{
  return species_;
}




compressibleMixtureThermophysicalProperties::SpeciesMassFractionList
compressibleMixtureThermophysicalProperties::defaultComposition() const
{
  // SpeciesMassFractionList defaultComposition;
  struct
      : public SpeciesMassFractionList,
        public boost::static_visitor<void>
  {
        void operator()(const Parameters::composition_fromFile_type& ff)
        {
            for (auto &s: ff.defaultMassFractions)
            {
                emplace(
                    s.first,
                    std::min(1., std::max<double>(
                        0., s.second ) ) );
            }
        }

        void operator()(const Parameters::composition_fromList_type& fl)
        {
            for (auto &s: fl.species)
            {
                emplace(
                    s.first,
                    std::min(1., std::max<double>(0.,
                        s.second.defaultMassFraction ) ) );
            }
        }

  } defaultComposition;
  boost::apply_visitor(defaultComposition, p().composition);

  // some species might have been removed explicitly
  std::set<std::string> tbr;
  for (auto& s: defaultComposition)
  {
      if (species_.count(s.first)<1)
          tbr.insert(s.first);
  }
  for (auto& s:tbr)
  {
      defaultComposition.erase(s);
  }

  // add zero for those yet unmentioned and check the sum
  auto sns=speciesNames();
  double w=0.;
  for (const auto &s: sns)
  {      
      if (!defaultComposition.count(s))
          defaultComposition.emplace(s, 0.);

      w+=defaultComposition.at(s);
  }

  insight::assertion(
    w<=1.,
    "mass fractions in default composition add up to more than unity (sum is %g)", w );

  if (w<1.)
  {
      defaultComposition[p().inertSpecie]=1.-w;
  }

  return defaultComposition;
}




void compressibleMixtureThermophysicalProperties::removeSpecie(
    const std::string& name)
{
  auto s = species_.find(name);

  if (s==species_.end())
    throw insight::Exception("Specie "+name+" was not foound in the composition!");

  if (const auto* sd = boost::get<SpeciesData>(&(s->second)))
  {
    species_.erase(s);
  }
  else
  {
    throw insight::Exception("Modfication of the species composition is only supported, if the fromFile option is not used!");
  }
}




void compressibleMixtureThermophysicalProperties::addSpecie(
    const std::string& name, const SpeciesData& d)
{
  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    species_[name]=d;
  }
  else
  {
    throw insight::Exception("Modfication of the species composition is only supported, if the fromFile option is not used!");
  }
}




std::string compressibleMixtureThermophysicalProperties::getTransportType() const
{
  std::string tn="";

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    for (const auto& s: list->species)
    {
      std::string ttn=SpeciesData(s.second.properties).transportType();

      if (tn.empty())
      {
        tn=ttn;
      }
      else
      {
        if (tn!=ttn)
          throw insight::Exception(
                "All species in the list have to use the same type of transport."
                " The first specie uses "+tn+" while "+s.first+" uses "+ttn+".");
      }
    }
  }

  return tn;
}




std::string compressibleMixtureThermophysicalProperties
    ::getThermoType() const
{
  std::string tn="";

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    for (const auto& s: list->species)
    {
      std::string ttn=SpeciesData(s.second.properties).thermoType();

      if (tn.empty())
      {
        tn=ttn;
      }
      else
      {
        if (tn!=ttn)
          throw insight::Exception(
                "All species in the list have to use the same type of thermo."
                " The first specie uses "+tn+" while "+s.first+" uses "+ttn+".");
      }
    }
  }

  return tn;
}


compressibleMixtureThermophysicalProperties::compressibleMixtureThermophysicalProperties(
    OpenFOAMCase& c, ParameterSetInput ip)
: thermodynamicModel(c, ip.forward<Parameters>())
{
  // ====================================================================================
  // ======== sanity check

  if (const auto * l =
        boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    if (l->species.size()<1)
    {
      throw insight::Exception("This list of species is empty. At least one specie has to be defined!");
    }

    {
      bool inertSpecieFound=false;
      for (const auto& s: l->species)
      {
        if (s.first==p().inertSpecie)
          inertSpecieFound=true;
      }
      if (!inertSpecieFound)
        throw insight::Exception("Inert specie "+p().inertSpecie+" was not in the list of defined species!");
    }

    // these include a check
    getTransportType();
    getThermoType();
  }

  // ====================================================================================
  // ======== load composition

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    for (const auto& l: list->species)
    {
      species_[l.first]=SpeciesData(l.second.properties);
    }
  }
  else if (const auto * file = boost::get<Parameters::composition_fromFile_type>(&p().composition))
  {
    std::istream& tf = (file->foamChemistryThermoFile)->stream();

    OFDictData::dict td;
    readOpenFOAMDict(tf, td);
    for (auto de=td.begin(); de!=td.end(); ++de)
    {
      species_[de->first]=boost::blank();
    }
  }
}




void compressibleMixtureThermophysicalProperties::addFields(OpenFOAMCase &c) const
{
  c.addField(
        "Ydefault",
        FieldInfo(scalarField,
                  dimless,
                  FieldValue({0.0}),
                  volField ) );

  auto dc=defaultComposition();
  for (const auto specie: dc)
  {
    c.addField(
          specie.first,
          FieldInfo(scalarField,
                    dimless,
                    FieldValue({specie.second}),
                    volField ) );
  }
}




void compressibleMixtureThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=
        dictionaries.lookupDict(
         "constant/thermophysicalProperties");

  const FVNumerics* nce = OFcase().get<FVNumerics>("FVNumerics");

  std::string ttn;
  if ( auto *rfn =
      dynamic_cast<const reactingFoamNumerics*>(
          nce ) )
  {
    if (rfn->p().buoyancy)
    {
      ttn="heRhoThermo";
    }
    else
    {
      ttn="hePsiThermo";
    }
  }
  else
  {
    throw insight::Exception("incompatible numerics selected!");
  }
  
  if (OFversion()<200)
  {
    throw insight::Exception("Unsupported OpenFOAM version!");
  }
  else
  {
    OFDictData::dict tt;
    tt["type"]=ttn;
    tt["mixture"]="reactingMixture";
    tt["transport"]=getTransportType();
    tt["thermo"]=getThermoType();
    tt["energy"]="sensibleEnthalpy";
    tt["equationOfState"]="perfectGas";
    tt["specie"]="specie";
    thermodynamicProperties["thermoType"]=tt;
  }

  
  thermodynamicProperties["inertSpecie"]=p().inertSpecie;
  thermodynamicProperties["chemistryReader"]="foamChemistryReader";




  if (const auto *file = boost::get<Parameters::reactions_fromFile_type>(&p().reactions))
  {
    thermodynamicProperties["foamChemistryFile"]=
        "\"" + dictionaries.insertAdditionalInputFile(
            file->foamChemistryFile).string()
        + "\"";
  }
  else
  {
    std::string chemfile="constant/reactions";
    auto& cd = dictionaries.lookupDict(chemfile);

    if (const auto *none = boost::get<Parameters::reactions_none_type>(&p().reactions))
    {
      OFDictData::list sl;
      auto sns=speciesNames();
      std::copy(sns.begin(), sns.end(), std::back_inserter(sl));
      cd["species"]=sl;

      if (OFversion()<200)
      {
        cd["reactions"]=OFDictData::list();
      }
      else
      {
        cd["reactions"]=OFDictData::dict();
      }
    }

    thermodynamicProperties["foamChemistryFile"]=
        "\"$FOAM_CASE/"+chemfile+"\"";
  }


  if (const auto *list = boost::get<Parameters::composition_fromList_type>(&p().composition))
  {
    std::string dictName("constant/thermo.compressibleGas");
    thermodynamicProperties["foamChemistryThermoFile"]=
        "\"$FOAM_CASE/"+dictName+"\"";

    auto& td = dictionaries.lookupDict(dictName);

    if (OFversion()>=200)
    {
      for (const auto& s: species_)
      {
        auto name = s.first;
        auto sd = boost::get<SpeciesData>(s.second);

        OFDictData::dict tsd;
        sd.insertSpecieEntries(tsd);
        sd.insertThermodynamicsEntries(tsd);
        sd.insertTransportEntries(tsd);
        sd.insertElementsEntries(tsd);
        td[name]=tsd;
      }
    }

  }
  else if (const auto *file = boost::get<Parameters::composition_fromFile_type>(&p().composition))
  {
    thermodynamicProperties["foamChemistryThermoFile"]=
        "\""+dictionaries.insertAdditionalInputFile(
            file->foamChemistryThermoFile).string()
        + "\"";
  }


  OFDictData::dict& combustionProperties=
      dictionaries.lookupDict("constant/combustionProperties");

  if (OFversion()<200)
  {
    if (const auto *pasr = boost::get<Parameters::combustionModel_PaSR_type>(&p().combustionModel))
    {
      combustionProperties["combustionModel"]="PaSR<psiChemistryCombustion>";
      combustionProperties["active"]=true;

      OFDictData::dict pd;
      pd["Cmix"]=pasr->Cmix;
      pd["turbulentReaction"]=true;
      combustionProperties["PaSRCoeffs"]=pd;

      OFDictData::dict& chemistryProperties=
          dictionaries.lookupDict("constant/chemistryProperties");

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
    else
    {
      throw insight::Exception(
            "unsupported combustion model for the selected OpenFOAM version");
    }
  }
  else if (OFversion()>=600)
  {
    if (boost::get<Parameters::combustionModel_none_type>(&p().combustionModel))
    {
      combustionProperties["combustionModel"]="none";
      combustionProperties["active"]=false;
    }
    else if (const auto *edc = boost::get<Parameters::combustionModel_EDC_type>(&p().combustionModel))
    {
      combustionProperties["combustionModel"]="EDC";
      OFDictData::dict ec;
      ec["version"]="v2005";
      combustionProperties["EDCCoeffs"]=ec;
      combustionProperties["active"]=true;
    }
    else
    {
      throw insight::Exception("unsupported combustion model for the selected OpenFOAM version");
    }
  }
  else
  {
    throw insight::UnsupportedFeature("unsupported OpenFOAM version");
  }
}

}

