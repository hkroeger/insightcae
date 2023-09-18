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

#include "base/boost_include.h"

#include "openfoam/openfoamcase.h"
#include "openfoam/openfoamtools.h"

#include "openfoam/caseelements/numerics/reactingfoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantsimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/buoyantpimplefoamnumerics.h"
#include "openfoam/caseelements/numerics/steadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/unsteadycompressiblenumerics.h"
#include "openfoam/caseelements/numerics/chtmultiregionnumerics.h"

#include <utility>


using namespace std;
using namespace boost;
using namespace boost::filesystem;
using namespace boost::assign;
using namespace boost::fusion;


namespace insight
{

cavitatingFoamThermodynamics::cavitatingFoamThermodynamics(OpenFOAMCase& c, const ParameterSet& ps)
: thermodynamicModel(c, ps),
  p_(ps)
{}




void cavitatingFoamThermodynamics::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.lookupDict("constant/thermodynamicProperties");
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






double sutherland_As(double mu, double Ts)
{
  return 2.*mu / sqrt(Ts);
}

double sutherland_mu(double As, double Ts)
{
  return As*sqrt(Ts)/2.;
}




const std::map<std::string, SpeciesData::ElementData> SpeciesData::elements = {
   { "H", {1, "Hydrogen", 1.008 } },
   { "He", {2, "Helium", 4.002602 } },
   { "Li", {3, "Lithium", 6.94 } },
   { "Be", {4, "Beryllium", 9.0121831 } },
   { "B", {5, "Boron", 10.81 } },
   { "C", {6, "Carbon", 12.011 } },
   { "N", {7, "Nitrogen", 14.007 } },
   { "O", {8, "Oxygen", 15.999 } },
   { "F", {9, "Fluorine", 18.998403163 } },
   { "Ne", {10, "Neon", 20.1797 } },
   { "Na", {11, "Sodium", 22.989769 } },
   { "Mg", {12, "Magnesium", 24.305 } },
   { "Al", {13, "Aluminium", 26.981538 } },
   { "Si", {14, "Silicon", 28.085 } },
   { "P", {15, "Phosphorus", 30.973761998 } },
   { "S", {16, "Sulfur", 32.06 } },
   { "Cl", {17, "Chlorine", 35.45 } },
   { "Ar", {18, "Argon", 39.948 } },
   { "K", {19, "Potassium", 39.0983 } },
   { "Ca", {20, "Calcium", 40.078 } },
   { "Sc", {21, "Scandium", 44.955908 } },
   { "Ti", {22, "Titanium", 47.867 } },
   { "V", {23, "Vanadium", 50.9415 } },
   { "Cr", {24, "Chromium", 51.9961 } },
   { "Mn", {25, "Manganese", 54.938043 } },
   { "Fe", {26, "Iron", 55.845 } },
   { "Co", {27, "Cobalt", 58.933194 } },
   { "Ni", {28, "Nickel", 58.6934 } },
   { "Cu", {29, "Copper", 63.546 } },
   { "Zn", {30, "Zinc", 65.38 } },
   { "Ga", {31, "Gallium", 69.723 } },
   { "Ge", {32, "Germanium", 72.63 } },
   { "As", {33, "Arsenic", 74.921595 } },
   { "Se", {34, "Selenium", 78.971 } },
   { "Br", {35, "Bromine", 79.904 } },
   { "Kr", {36, "Krypton", 83.798 } },
   { "Rb", {37, "Rubidium", 85.4678 } },
   { "Sr", {38, "Strontium", 87.62 } },
   { "Y", {39, "Yttrium", 88.90584 } },
   { "Zr", {40, "Zirconium", 91.224 } },
   { "Nb", {41, "Niobium", 92.90637 } },
   { "Mo", {42, "Molybdenum", 95.95 } },
   { "Tc", {43, "Technetium", 97 } },
   { "Ru", {44, "Ruthenium", 101.07 } },
   { "Rh", {45, "Rhodium", 102.90549 } },
   { "Pd", {46, "Palladium", 106.42 } },
   { "Ag", {47, "Silver", 107.8682 } },
   { "Cd", {48, "Cadmium", 112.414 } },
   { "In", {49, "Indium", 114.818 } },
   { "Sn", {50, "Tin", 118.71 } },
   { "Sb", {51, "Antimony", 121.76 } },
   { "Te", {52, "Tellurium", 127.6 } },
   { "I", {53, "Iodine", 126.90447 } },
   { "Xe", {54, "Xenon", 131.293 } },
   { "Cs", {55, "Caesium", 132.90545196 } },
   { "Ba", {56, "Barium", 137.327 } },
   { "La", {57, "Lanthanum", 138.90547 } },
   { "Ce", {58, "Cerium", 140.116 } },
   { "Pr", {59, "Praseodymium", 140.90766 } },
   { "Nd", {60, "Neodymium", 144.242 } },
   { "Pm", {61, "Promethium", 145 } },
   { "Sm", {62, "Samarium", 150.36 } },
   { "Eu", {63, "Europium", 151.964 } },
   { "Gd", {64, "Gadolinium", 157.25 } },
   { "Tb", {65, "Terbium", 158.925354 } },
   { "Dy", {66, "Dysprosium", 162.5 } },
   { "Ho", {67, "Holmium", 164.930328 } },
   { "Er", {68, "Erbium", 167.259 } },
   { "Tm", {69, "Thulium", 168.934218 } },
   { "Yb", {70, "Ytterbium", 173.045 } },
   { "Lu", {71, "Lutetium", 174.9668 } },
   { "Hf", {72, "Hafnium", 178.486 } },
   { "Ta", {73, "Tantalum", 180.94788 } },
   { "W", {74, "Tungsten", 183.84 } },
   { "Re", {75, "Rhenium", 186.207 } },
   { "Os", {76, "Osmium", 190.23 } },
   { "Ir", {77, "Iridium", 192.217 } },
   { "Pt", {78, "Platinum", 195.084 } },
   { "Au", {79, "Gold", 196.96657 } },
   { "Hg", {80, "Mercury", 200.592 } },
   { "Tl", {81, "Thallium", 204.38 } },
   { "Pb", {82, "Lead", 207.2 } },
   { "Bi", {83, "Bismuth", 208.9804 } },
   { "Po", {84, "Polonium", 209 } },
   { "At", {85, "Astatine", 210 } },
   { "Rn", {86, "Radon", 222 } },
   { "Fr", {87, "Francium", 223 } },
   { "Ra", {88, "Radium", 226 } },
   { "Ac", {89, "Actinium", 227 } },
   { "Th", {90, "Thorium", 232.0377 } },
   { "Pa", {91, "Protactinium", 231.03588 } },
   { "U", {92, "Uranium", 238.02891 } },
   { "Np", {93, "Neptunium", 237 } },
   { "Pu", {94, "Plutonium", 244 } },
   { "Am", {95, "Americium", 243 } },
   { "Cm", {96, "Curium", 247 } },
   { "Bk", {97, "Berkelium", 247 } },
   { "Cf", {98, "Californium", 251 } },
   { "Es", {99, "Einsteinium", 252 } },
   { "Fm", {100, "Fermium", 257 } },
   { "Md", {101, "Mendelevium", 258 } },
   { "No", {102, "Nobelium", 259 } },
   { "Lr", {103, "Lawrencium", 262 } },
   { "Rf", {104, "Rutherfordium", 267 } },
   { "Db", {105, "Dubnium", 270 } },
   { "Sg", {106, "Seaborgium", 269 } },
   { "Bh", {107, "Bohrium", 270 } },
   { "Hs", {108, "Hassium", 270 } },
   { "Mt", {109, "Meitnerium", 278 } },
   { "Ds", {110, "Darmstadtium", 281 } },
   { "Rg", {111, "Roentgenium", 281 } },
   { "Cn", {112, "Copernicium", 285 } },
   { "Nh", {113, "Nihonium", 286 } },
   { "Fl", {114, "Flerovium", 289 } },
   { "Mc", {115, "Moscovium", 289 } },
   { "Lv", {116, "Livermorium", 293 } },
   { "Ts", {117, "Tennessine", 293 } },
   { "Og", {118, "Oganesson", 294 } }
};


void SpeciesData::modifyDefaults(ParameterSet &ps)
{
  auto& p = ps.get<SelectableSubsetParameter>("properties");
  auto& fl = p.items().at("fromLibrary");
  auto& sl = fl->get<SelectionParameter>("specie");

  if (speciesLibrary_.size()>0)
  {
    sl.items().clear();

    std::transform(speciesLibrary_.begin(), speciesLibrary_.end(),
                   std::back_inserter(sl.items()),
                   [](const SpeciesLibrary::value_type& sle)
                    {
                      return sle.first;
                    }
    );
  }

}

SpeciesData::SpeciesData(const ParameterSet &ps)
{
  Parameters p(ps);

  name_=p.name;

  const auto& propsel = ps.get<SelectableSubsetParameter>("properties");

  if (propsel.selection()=="fromLibrary")
  {
    const auto& specie_sel = propsel().get<SelectionParameter>("specie");

    auto sname = specie_sel.selection();

    auto e = speciesLibrary_.find(sname);
    if (e==speciesLibrary_.end())
    {
      throw insight::Exception("invalid selection: species library does not contain "+sname);
    }
    else
    {
      p_=e->second;
    }
  }
  else if (const auto* c = boost::get<Parameters::properties_custom_type>(&p.properties))
  {
    p_=*c;
  }
}


SpeciesData::SpeciesData(
    const std::string& name,
    std::vector<std::pair<double, SpeciesData> > mixture
    )
  : name_(name)
{
  Parameters::properties_custom_type::thermo_janaf_type mixthermo;
  std::fill(mixthermo.coeffs_hi.begin(), mixthermo.coeffs_hi.end(), 0.);
  std::fill(mixthermo.coeffs_lo.begin(), mixthermo.coeffs_lo.end(), 0.);

  Parameters::properties_custom_type::transport_sutherland_type mixtrans;
  double mix_st_A=0.0;
  mixtrans.Tref=0.0;

  Parameters::properties_custom_type::elements_type elements;

  double wtotal=0.0;

  for (auto s=mixture.begin(); s!=mixture.end(); ++s)
  {
    auto sp=s->second.p_;

    wtotal += s->first;

    if (const auto *jt = boost::get<Parameters::properties_custom_type::thermo_janaf_type>(&sp.thermo))
    {
      if (s==mixture.begin())
      {
        mixthermo.Tlow=jt->Tlow;
        mixthermo.Thi=jt->Thi;
        mixthermo.Tmid=jt->Tmid;
      }
      else
      {
        mixthermo.Tlow=std::max(mixthermo.Tlow, jt->Tlow);
        mixthermo.Thi=std::max(mixthermo.Thi, jt->Thi);

        if (
            (fabs(mixthermo.Tmid-jt->Tmid)>1e-3)
            )
          throw insight::Exception("Cannot lump species: middle temperature of janaf polynomials do not match for all species!");
      }

      insight::assertion(mixthermo.coeffs_lo.size()==jt->coeffs_lo.size(), "length of low temperature janaf coefficient arrays must match");
      insight::assertion(mixthermo.coeffs_hi.size()==jt->coeffs_hi.size(), "length of high temperature janaf coefficient arrays must match");

      for (size_t i=0; i<mixthermo.coeffs_lo.size(); i++)
      {
        mixthermo.coeffs_lo[i] += s->first * jt->coeffs_lo[i];
        mixthermo.coeffs_hi[i] += s->first * jt->coeffs_hi[i];
      }
    }
    else
    {
      throw insight::Exception("Can only lump mixtures which homogeneously use janaf polynomials.");
    }

    if (const auto *st = boost::get<Parameters::properties_custom_type::transport_sutherland_type>(&sp.transport))
    {
      mix_st_A += s->first * sutherland_As(st->mu, st->Tref);
      mixtrans.Tref += s->first * st->Tref;
    }
    else
    {
      throw insight::Exception("Can only lump mixtures which homogeneously use sutherland transport.");
    }

    for (const auto& e: sp.elements)
    {
      double nj=e.number/s->first;

      bool wasPresent=false;
      for (auto& ee: elements)
      {
        if (ee.element==e.element)
        {
          ee.number += nj;
          wasPresent=true;
        }
      }
      if (!wasPresent)
      {
        Parameters::properties_custom_type::elements_default_type eee;
        eee.element=e.element;
        eee.number=nj;
        elements.push_back(eee);
      }
    }

  }

  if (fabs(wtotal-1.0)>1e-3)
    throw insight::Exception("The mass fraction of the species in the lumped mixture does not add to 1!");

  p_.thermo=mixthermo;

  mixtrans.mu = sutherland_mu(mix_st_A, mixtrans.Tref);
  p_.transport=mixtrans;

  p_.elements=elements;
}


double SpeciesData::M() const
{
  double result=0;

  for(const auto& e: p_.elements)
  {
    auto ed = elements.find(e.element);

    if (ed==elements.end())
      throw insight::Exception("Unknown element "+e.element);

    result += e.number * ed->second.M;
  }

  return result;
}

std::string SpeciesData::transportType() const
{
  if (const auto *ct = boost::get<Parameters::properties_custom_type::transport_constant_type>(&p_.transport))
  {
    return "const";
  }
  else if (const auto *st = boost::get<Parameters::properties_custom_type::transport_sutherland_type>(&p_.transport))
  {
    return "sutherland";
  }
  return "";
}

std::string SpeciesData::thermoType() const
{
  if (const auto *ct = boost::get<Parameters::properties_custom_type::thermo_constant_type>(&p_.thermo))
  {
    return "hConst";
  }
  else if (const auto *jt = boost::get<Parameters::properties_custom_type::thermo_janaf_type>(&p_.thermo))
  {
    return "janaf";
  }
  return "";
}

string SpeciesData::equationOfStateType() const
{
  if (boost::get<Parameters::properties_custom_type::equationOfState_perfectGas_type>(
          &p_.equationOfState))
  {
    return "perfectGas";
  }
  else if (boost::get<Parameters::properties_custom_type::equationOfState_perfectFluid_type>(
          &p_.equationOfState))
  {
    return "perfectFluid";
  }
  else if (boost::get<Parameters::properties_custom_type::equationOfState_adiabaticPerfectFluid_type>(
               &p_.equationOfState))
  {
    return "adiabaticPerfectFluid";
  }
  else if (boost::get<Parameters::properties_custom_type::equationOfState_rPolynomial_type>(
               &p_.equationOfState))
  {
    return "rPolynomial";
  }
  else
  {
    throw insight::Exception("Unhandled equationOfState type!");
  }
  return "";
}

void SpeciesData::insertSpecieEntries(OFDictData::dict& d) const
{
  OFDictData::dict specie;
  specie["molWeight"]=M();
  d["specie"]=specie;
}

std::pair<double,double> SpeciesData::temperatureLimits() const
{
  std::pair<double,double> mima(0, 1e10);
  if (const auto *jt = boost::get<Parameters::properties_custom_type::thermo_janaf_type>(&p_.thermo))
  {
    mima.first=jt->Tlow;
    mima.second=jt->Thi;
  }
  return mima;
}

void SpeciesData::insertThermodynamicsEntries(OFDictData::dict& d) const
{
  OFDictData::dict thermodynamics;
  if (const auto *ct = boost::get<Parameters::properties_custom_type::thermo_constant_type>(&p_.thermo))
  {
    thermodynamics["Cp"] = ct->Cp;
    thermodynamics["Hf"] = ct->Hf;
  }
  else if (const auto *jt = boost::get<Parameters::properties_custom_type::thermo_janaf_type>(&p_.thermo))
  {
    thermodynamics["Tlow"]=jt->Tlow;
    thermodynamics["Thigh"]=jt->Thi;
    thermodynamics["Tcommon"]=jt->Tmid;

    {
      OFDictData::list cpc;
      for (arma::uword i=0; i<jt->coeffs_hi.size(); i++)
      {
        cpc.push_back(jt->coeffs_hi(i));
      }
      thermodynamics["highCpCoeffs"]=cpc;
    }

    {
      OFDictData::list cpc;
      for (arma::uword i=0; i<jt->coeffs_lo.size(); i++)
      {
        cpc.push_back(jt->coeffs_lo(i));
      }
      thermodynamics["lowCpCoeffs"]=cpc;
    }
  }
  d["thermodynamics"]=thermodynamics;
}


void SpeciesData::insertTransportEntries(OFDictData::dict& d) const
{
  OFDictData::dict transport;
  if (const auto *ct = boost::get<Parameters::properties_custom_type::transport_constant_type>(&p_.transport))
  {
    transport["Pr"]=ct->Pr;
    transport["mu"]=ct->mu;
  }
  else if (const auto *st = boost::get<Parameters::properties_custom_type::transport_sutherland_type>(&p_.transport))
  {
    transport["Ts"]=st->Tref;
    transport["As"]=sutherland_As(st->mu, st->Tref);
    transport["Pr"]=st->Pr;
  }
  d["transport"]=transport;
}


void SpeciesData::insertEquationOfStateEntries(
    OFDictData::dict &d ) const
{
  if (boost::get<Parameters::properties_custom_type::equationOfState_perfectGas_type>(
          &p_.equationOfState))
  {
  }
  else if (const auto * pf =
             boost::get<Parameters::properties_custom_type::equationOfState_perfectFluid_type>(
               &p_.equationOfState))
  {
    OFDictData::dict eos;
    eos["R"]=pf->R;
    eos["rho0"]=pf->rho0;
    d["equationOfState"]=eos;
  }
  else if (const auto * apf =
           boost::get<Parameters::properties_custom_type::equationOfState_adiabaticPerfectFluid_type>(
               &p_.equationOfState))
  {
    OFDictData::dict eos;
    eos["p0"]=apf->p0;
    eos["rho0"]=apf->rho0;
    eos["B"]=apf->B;
    eos["gamma"]=apf->gamma;
    d["equationOfState"]=eos;
  }
  else if (const auto * rp =
           boost::get<Parameters::properties_custom_type::equationOfState_rPolynomial_type>(
               &p_.equationOfState))
  {
    OFDictData::dict eos;
    eos["C"]=OFDictData::list(rp->C);
    d["equationOfState"]=eos;
  }
  else
  {
    throw insight::Exception("Unhandled equationOfState type!");
  }
}


void SpeciesData::insertElementsEntries(OFDictData::dict& d) const
{
  OFDictData::dict elements;
  for (const auto& e: p_.elements)
  {
    elements[e.element]=e.number;
  }
  d["elements"]=elements;
}



SpeciesData::Parameters::properties_custom_type
SpeciesData::specieFromLibrary(const std::string &name)
{
  auto i = speciesLibrary_.find(name);

  insight::assertion(
      i != speciesLibrary_.end(),
      "specie %s not found in library!", name.c_str());

  return i->second;
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
      dynamic_cast<const buoyantPimpleFoamNumerics*>(nce) ||
      dynamic_cast<const chtMultiRegionFluidNumerics*>(nce)
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
      if (unsteadyCompressibleNumerics::Parameters(t->parameters()).formulation ==
            unsteadyCompressibleNumerics::Parameters::sonicFoam)
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
      dictionaries.lookupDict("constant/thermophysicalProperties");


  if (OFversion()<170)
  {
    std::string tt = requiredThermoType();

    tt += "<pureMixture<";

    std::string mixp_eqn, mixp_thermo, mixp_transp, mixp =
        boost::str(boost::format("specie 1 %g") % p_.M);

    if (const auto *ct = boost::get<Parameters::transport_constant_type>(&p_.transport))
    {
      tt+="constTransport";
      mixp_transp =
          boost::str(boost::format("%g %g") % ct->mu % ct->Pr );
    }
    else if (const auto *st = boost::get<Parameters::transport_sutherland_type>(&p_.transport))
    {
      tt+="sutherlandTransport";
      mixp_transp =
          boost::str(boost::format("%g %g") % sutherland_As(st->mu, st->Tref) % st->Tref );
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
    else
    {
      throw insight::Exception("Unsupported equation of state!");
    }

    tt+=">>>>>";

    thermophysicalProperties["thermoType"]=tt;
    thermophysicalProperties["mixture"]=mixp +"\n "+ mixp_eqn +"\n "+ mixp_thermo +"\n "+ mixp_transp;
  }
  else
  {

    OFDictData::dict thermoType,
        mixture_specie, mixture_thermodynamics,
        mixture_transport, mixture_equationOfState;

    thermoType["type"]=requiredThermoType();
    thermoType["specie"]="specie";
    thermoType["energy"]="sensibleEnthalpy";
    thermoType["mixture"]="pureMixture";

    mixture_specie["molWeight"]=p_.M;

    if (const auto *ct = boost::get<Parameters::transport_constant_type>(&p_.transport))
    {
      thermoType["transport"]="const";
      mixture_transport["Pr"]=ct->Pr;
      mixture_transport["mu"]=ct->mu;
    }
    else if (const auto *st = boost::get<Parameters::transport_sutherland_type>(&p_.transport))
    {
      thermoType["transport"]="sutherland";
      mixture_transport["Ts"]=st->Tref;
      mixture_transport["As"]=sutherland_As(st->mu, st->Tref);
      mixture_transport["Pr"]=st->Pr;
    }

    if (const auto *ct = boost::get<Parameters::thermo_constant_type>(&p_.thermo))
    {
      thermoType["thermo"]="hConst";
      mixture_thermodynamics["Cp"] = ct->Cp;
      mixture_thermodynamics["Hf"] = ct->Hf;
    }
    else if (const auto *jt = boost::get<Parameters::thermo_janaf_type>(&p_.thermo))
    {
      thermoType["thermo"]="janaf";
      mixture_thermodynamics["Tlow"]=jt->Tlow;
      mixture_thermodynamics["Thigh"]=jt->Thi;
      mixture_thermodynamics["Tcommon"]=jt->Tmid;

      {
        OFDictData::list cpc;
        for (arma::uword i=0; i<jt->coeffs_hi.size(); i++)
        {
          cpc.push_back(jt->coeffs_hi(i));
        }
        mixture_thermodynamics["highCpCoeffs"]=cpc;
      }

      {
        OFDictData::list cpc;
        for (arma::uword i=0; i<jt->coeffs_lo.size(); i++)
        {
          cpc.push_back(jt->coeffs_lo(i));
        }
        mixture_thermodynamics["lowCpCoeffs"]=cpc;
      }
    }


    if (const auto *pe = boost::get<Parameters::equationOfState_idealGas_type>(&p_.equationOfState))
    {
      thermoType["equationOfState"]="perfectGas";
    }
    else if (const auto *pre = boost::get<Parameters::equationOfState_PengRobinson_type>(&p_.equationOfState))
    {
      thermoType["equationOfState"]="PengRobinsonGas";
      mixture_equationOfState["Tc"]=pre->Tc;
      mixture_equationOfState["Vc"]=0.0;
      mixture_equationOfState["Pc"]=pre->Pc;
      mixture_equationOfState["omega"]=pre->omega;
    }
    else if (const auto *rc = boost::get<Parameters::equationOfState_rhoConst_type>(&p_.equationOfState))
    {
      thermoType["equationOfState"]="rhoConst";
      mixture_equationOfState["rho"]=rc->rho;
    }
    else
    {
      throw insight::Exception("Unsupported equation of state!");
    }


    thermophysicalProperties["thermoType"]=thermoType;
    OFDictData::dict mixdict;
    mixdict["specie"]=mixture_specie;
    mixdict["thermodynamics"]=mixture_thermodynamics;
    mixdict["transport"]=mixture_transport;
    mixdict["equationOfState"]=mixture_equationOfState;
    thermophysicalProperties["mixture"]=mixdict;
  }

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

//  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
//  {
//    std::transform(list->species.begin(), list->species.end(),
//                   std::back_inserter(sns),
//                   [](const Parameters::composition_fromList_type::species_default_type& e)
//                    {
//                      return e.name;
//                    }
//    );
//  }
//  else if (const auto * file = boost::get<Parameters::composition_fromFile_type>(&p_.composition))
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

compressibleMixtureThermophysicalProperties::SpeciesList compressibleMixtureThermophysicalProperties::species() const
{
  return species_;
}


compressibleMixtureThermophysicalProperties::SpeciesMassFractionList
compressibleMixtureThermophysicalProperties::defaultComposition() const
{
  SpeciesMassFractionList defaultComposition;
  auto sns=speciesNames();
  for (const auto specie: sns)
  {
    double v=0.0;
    if (specie==p_.inertSpecie) v=1.0;
    defaultComposition[specie]=v;
  }
  return defaultComposition;
}



void compressibleMixtureThermophysicalProperties::removeSpecie(const std::string& name)
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

void compressibleMixtureThermophysicalProperties::addSpecie(const std::string& name, SpeciesData d)
{
  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
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

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
  {
    for (const auto& s: list->species)
    {
      std::string ttn=SpeciesData(s).transportType();

      if (tn.empty())
      {
        tn=ttn;
      }
      else
      {
        if (tn!=ttn)
          throw insight::Exception("All species in the list have to use the same type of transport."
                                   " The first specie uses "+tn+" while "+s.name+" uses "+ttn+".");
      }
    }
  }

  return tn;
}

std::string compressibleMixtureThermophysicalProperties::getThermoType() const
{
  std::string tn="";

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
  {
    for (const auto& s: list->species)
    {
      std::string ttn=SpeciesData(s).thermoType();

      if (tn.empty())
      {
        tn=ttn;
      }
      else
      {
        if (tn!=ttn)
          throw insight::Exception("All species in the list have to use the same type of thermo."
                                   " The first specie uses "+tn+" while "+s.name+" uses "+ttn+".");
      }
    }
  }

  return tn;
}

compressibleMixtureThermophysicalProperties::compressibleMixtureThermophysicalProperties
(
  OpenFOAMCase& c, 
  const ParameterSet& ps
)
: thermodynamicModel(c, ps),
  p_(ps)
{

  // ====================================================================================
  // ======== sanity check

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
  {
    if (list->species.size()<1)
    {
      throw insight::Exception("This list of species is empty. At least one specie has to be defined!");
    }

    {
      bool inertSpecieFound=false;
      for (const auto& s: list->species)
      {
        if (s.name==p_.inertSpecie)
          inertSpecieFound=true;
      }
      if (!inertSpecieFound)
        throw insight::Exception("Inert specie "+p_.inertSpecie+" was not in the list of defined species!");
    }

    // these include a check
    getTransportType();
    getThermoType();
  }

  // ====================================================================================
  // ======== load composition

  if (const auto * list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
  {
    for (const auto& l: list->species)
    {
      species_[l.name]=SpeciesData(l);
    }
  }
  else if (const auto * file = boost::get<Parameters::composition_fromFile_type>(&p_.composition))
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
  c.addField("Ydefault", FieldInfo(scalarField, 	dimless, 	FieldValue({0.0}), volField ) );

  auto dc=defaultComposition();
  for (const auto specie: dc)
  {
    c.addField(specie.first, FieldInfo(scalarField, 	dimless, 	FieldValue({specie.second}), volField ) );
  }
}




void compressibleMixtureThermophysicalProperties::addIntoDictionaries(OFdicts& dictionaries) const
{
  OFDictData::dict& thermodynamicProperties=dictionaries.lookupDict("constant/thermophysicalProperties");

  const FVNumerics* nce = OFcase().get<FVNumerics>("FVNumerics");

  std::string ttn;
  if ( const auto *rfn = dynamic_cast<const reactingFoamNumerics*>(nce) )
  {
    if (rfn->parameters().buoyancy)
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

  
  thermodynamicProperties["inertSpecie"]=p_.inertSpecie;
  thermodynamicProperties["chemistryReader"]="foamChemistryReader";




  if (const auto *file = boost::get<Parameters::reactions_fromFile_type>(&p_.reactions))
  {
    thermodynamicProperties["foamChemistryFile"]=
        "\""+dictionaries.insertAdditionalInputFile(file->foamChemistryFile).string()+"\"";
  }
  else
  {
    std::string chemfile="constant/reactions";
    auto& cd = dictionaries.lookupDict(chemfile);

    if (const auto *none = boost::get<Parameters::reactions_none_type>(&p_.reactions))
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

    thermodynamicProperties["foamChemistryFile"]="\"$FOAM_CASE/"+chemfile+"\"";
  }

  
  if (const auto *list = boost::get<Parameters::composition_fromList_type>(&p_.composition))
  {
    std::string dictName("constant/thermo.compressibleGas");
    thermodynamicProperties["foamChemistryThermoFile"]="\"$FOAM_CASE/"+dictName+"\"";

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
  else if (const auto *file = boost::get<Parameters::composition_fromFile_type>(&p_.composition))
  {
    thermodynamicProperties["foamChemistryThermoFile"]=
        "\""+dictionaries.insertAdditionalInputFile(file->foamChemistryThermoFile).string()+"\"";
  }


  OFDictData::dict& combustionProperties=dictionaries.lookupDict("constant/combustionProperties");

  if (OFversion()<200)
  {
    if (const auto *pasr = boost::get<Parameters::combustionModel_PaSR_type>(&p_.combustionModel))
    {
      combustionProperties["combustionModel"]="PaSR<psiChemistryCombustion>";
      combustionProperties["active"]=true;

      OFDictData::dict pd;
      pd["Cmix"]=pasr->Cmix;
      pd["turbulentReaction"]=true;
      combustionProperties["PaSRCoeffs"]=pd;

      OFDictData::dict& chemistryProperties=dictionaries.lookupDict("constant/chemistryProperties");

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
      throw insight::Exception("unsupported combustion model for the selected OpenFOAM version");
    }
  }
  else if (OFversion()>=600)
  {
    if (boost::get<Parameters::combustionModel_none_type>(&p_.combustionModel))
    {
      combustionProperties["combustionModel"]="none";
      combustionProperties["active"]=false;
    }
    else if (const auto *edc = boost::get<Parameters::combustionModel_EDC_type>(&p_.combustionModel))
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

SpeciesData::SpeciesLibrary SpeciesData::speciesLibrary_;

SpeciesData::SpeciesLibrary::SpeciesLibrary()

{
  using namespace boost::filesystem;

  auto paths = SharedPathList::global();
  for ( const path& sharedPath: paths )
  {
    if ( exists(sharedPath) && is_directory (sharedPath) )
    {
      path spd=sharedPath/"thermophysical";

      if ( exists(spd) && is_directory ( spd ) )
      {
        directory_iterator end_itr; // default construction yields past-the-end

        for ( directory_iterator itr ( spd ); itr != end_itr; ++itr )
        {
          if ( is_regular_file ( itr->status() ) )
          {
            if ( itr->path().extension() == ".species" )
            {
              CurrentExceptionContext ex("reading species data base "+itr->path().string());
              std::ifstream fs(itr->path().string());
              OFDictData::dict sd;
              readOpenFOAMDict(fs, sd);

              for (const auto& s: sd)
              {
                CurrentExceptionContext ex2("reading species "+s.first);
                auto name=s.first;
                auto ssd=sd.subDict(s.first);

                Parameters::properties_custom_type data;
                Parameters::properties_custom_type::transport_sutherland_type trdata;
                Parameters::properties_custom_type::thermo_janaf_type thdata;

                {
                  auto& ed=ssd.subDict("elements");
                  data.elements.clear();
                  transform(ed.begin(), ed.end(),
                            std::back_inserter(data.elements),
                            [](const OFDictData::dict::value_type& e)
                  {
                    return Parameters::properties_custom_type::elements_default_type
                    {e.first, boost::get<int>(e.second)};
                  }
                  );
                }
                {
                  auto& td=ssd.subDict("transport");
                  trdata.Tref=td.getDouble("Ts");
                  trdata.mu=sutherland_mu(td.getDouble("As"), trdata.Tref);
                }
                {
                  auto& td=ssd.subDict("thermodynamics");
                  thdata.Tlow=td.getDouble("Tlow");
                  thdata.Thi=td.getDouble("Thigh");
                  thdata.Tmid=td.getDouble("Tcommon");
                  {
                    auto l=td.getList("lowCpCoeffs", false);
                    thdata.coeffs_lo.resize(l.size());
                    int i=0;
                    for (const auto& e: l)
                    {
                      if (auto *v = boost::get<double>(&e))
                            thdata.coeffs_lo(i)=*v;
                      else
                            thdata.coeffs_lo(i)=boost::get<int>(e);
                      i++;
                    }
                  }
                  {
                    auto h=td.getList("highCpCoeffs", false);
                    thdata.coeffs_hi.resize(h.size());
                    int i=0;
                    for (const auto& e: h)
                    {
                      if (auto *v = boost::get<double>(&e))
                        thdata.coeffs_hi(i)=*v;
                      else
                        thdata.coeffs_hi(i)=boost::get<int>(e);
                      i++;
                    }
                  }
                }

                data.transport=trdata;
                data.thermo=thdata;

                this->insert(value_type(name, data));
              }
            }
          }
        }
      }
    }
  }
}

}

