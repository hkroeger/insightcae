#include "speciesdata.h"

#include "base/exception.h"
#include "base/tools.h"


namespace insight {




double sutherland_As(double mu, double Ts)
{
    return 2.*mu / sqrt(Ts);
}




double sutherland_mu(double As, double Ts)
{
    return As*sqrt(Ts)/2.;
}




const std::map<std::string, SpeciesData::ElementData> SpeciesData::elements = {
    { "H",  {1, "Hydrogen",     1.008 } },
    { "He", {2, "Helium",       4.002602 } },
    { "Li", {3, "Lithium",      6.94 } },
    { "Be", {4, "Beryllium",    9.0121831 } },
    { "B",  {5, "Boron",        10.81 } },
    { "C",  {6, "Carbon",       12.011 } },
    { "N",  {7, "Nitrogen",     14.007 } },
    { "O",  {8, "Oxygen",       15.999 } },
    { "F",  {9, "Fluorine",     18.998403163 } },
    { "Ne", {10, "Neon",        20.1797 } },
    { "Na", {11, "Sodium",      22.989769 } },
    { "Mg", {12, "Magnesium",   24.305 } },
    { "Al", {13, "Aluminium",   26.981538 } },
    { "Si", {14, "Silicon",     28.085 } },
    { "P",  {15, "Phosphorus",  30.973761998 } },
    { "S",  {16, "Sulfur",      32.06 } },
    { "Cl", {17, "Chlorine",    35.45 } },
    { "Ar", {18, "Argon",       39.948 } },
    { "K",  {19, "Potassium",   39.0983 } },
    { "Ca", {20, "Calcium",     40.078 } },
    { "Sc", {21, "Scandium",    44.955908 } },
    { "Ti", {22, "Titanium",    47.867 } },
    { "V",  {23, "Vanadium",    50.9415 } },
    { "Cr", {24, "Chromium",    51.9961 } },
    { "Mn", {25, "Manganese",   54.938043 } },
    { "Fe", {26, "Iron",        55.845 } },
    { "Co", {27, "Cobalt",      58.933194 } },
    { "Ni", {28, "Nickel",      58.6934 } },
    { "Cu", {29, "Copper",      63.546 } },
    { "Zn", {30, "Zinc",        65.38 } },
    { "Ga", {31, "Gallium",     69.723 } },
    { "Ge", {32, "Germanium",   72.63 } },
    { "As", {33, "Arsenic",     74.921595 } },
    { "Se", {34, "Selenium",    78.971 } },
    { "Br", {35, "Bromine",     79.904 } },
    { "Kr", {36, "Krypton",     83.798 } },
    { "Rb", {37, "Rubidium",    85.4678 } },
    { "Sr", {38, "Strontium",   87.62 } },
    { "Y",  {39, "Yttrium",     88.90584 } },
    { "Zr", {40, "Zirconium",   91.224 } },
    { "Nb", {41, "Niobium",     92.90637 } },
    { "Mo", {42, "Molybdenum",  95.95 } },
    { "Tc", {43, "Technetium",  97. } },
    { "Ru", {44, "Ruthenium",   101.07 } },
    { "Rh", {45, "Rhodium",     102.90549 } },
    { "Pd", {46, "Palladium",   106.42 } },
    { "Ag", {47, "Silver",      107.8682 } },
    { "Cd", {48, "Cadmium",     112.414 } },
    { "In", {49, "Indium",      114.818 } },
    { "Sn", {50, "Tin",         118.71 } },
    { "Sb", {51, "Antimony",    121.76 } },
    { "Te", {52, "Tellurium",   127.6 } },
    { "I",  {53, "Iodine",      126.90447 } },
    { "Xe", {54, "Xenon",       131.293 } },
    { "Cs", {55, "Caesium",     132.90545196 } },
    { "Ba", {56, "Barium",      137.327 } },
    { "La", {57, "Lanthanum",   138.90547 } },
    { "Ce", {58, "Cerium",      140.116 } },
    { "Pr", {59, "Praseodymium", 140.90766 } },
    { "Nd", {60, "Neodymium",   144.242 } },
    { "Pm", {61, "Promethium",  145. } },
    { "Sm", {62, "Samarium",    150.36 } },
    { "Eu", {63, "Europium",    151.964 } },
    { "Gd", {64, "Gadolinium",  157.25 } },
    { "Tb", {65, "Terbium",     158.925354 } },
    { "Dy", {66, "Dysprosium",  162.5 } },
    { "Ho", {67, "Holmium",     164.930328 } },
    { "Er", {68, "Erbium",      167.259 } },
    { "Tm", {69, "Thulium",     168.934218 } },
    { "Yb", {70, "Ytterbium",   173.045 } },
    { "Lu", {71, "Lutetium",    174.9668 } },
    { "Hf", {72, "Hafnium",     178.486 } },
    { "Ta", {73, "Tantalum",    180.94788 } },
    { "W",  {74, "Tungsten",    183.84 } },
    { "Re", {75, "Rhenium",     186.207 } },
    { "Os", {76, "Osmium",      190.23 } },
    { "Ir", {77, "Iridium",     192.217 } },
    { "Pt", {78, "Platinum",    195.084 } },
    { "Au", {79, "Gold",        196.96657 } },
    { "Hg", {80, "Mercury",     200.592 } },
    { "Tl", {81, "Thallium",    204.38 } },
    { "Pb", {82, "Lead",        207.2 } },
    { "Bi", {83, "Bismuth",     208.9804 } },
    { "Po", {84, "Polonium",    209. } },
    { "At", {85, "Astatine",    210. } },
    { "Rn", {86, "Radon",       222. } },
    { "Fr", {87, "Francium",    223. } },
    { "Ra", {88, "Radium",      226. } },
    { "Ac", {89, "Actinium",    227. } },
    { "Th", {90, "Thorium",     232.0377 } },
    { "Pa", {91, "Protactinium", 231.03588 } },
    { "U",  {92, "Uranium",     238.02891 } },
    { "Np", {93, "Neptunium",   237. } },
    { "Pu", {94, "Plutonium",   244. } },
    { "Am", {95, "Americium",   243. } },
    { "Cm", {96, "Curium",      247. } },
    { "Bk", {97, "Berkelium",   247. } },
    { "Cf", {98, "Californium", 251. } },
    { "Es", {99, "Einsteinium", 252. } },
    { "Fm", {100, "Fermium",    257. } },
    { "Md", {101, "Mendelevium", 258. } },
    { "No", {102, "Nobelium",   259. } },
    { "Lr", {103, "Lawrencium", 262. } },
    { "Rf", {104, "Rutherfordium", 267. } },
    { "Db", {105, "Dubnium",    270. } },
    { "Sg", {106, "Seaborgium", 269. } },
    { "Bh", {107, "Bohrium",    270. } },
    { "Hs", {108, "Hassium",    270. } },
    { "Mt", {109, "Meitnerium", 278. } },
    { "Ds", {110, "Darmstadtium", 281. } },
    { "Rg", {111, "Roentgenium", 281. } },
    { "Cn", {112, "Copernicium", 285. } },
    { "Nh", {113, "Nihonium",   286. } },
    { "Fl", {114, "Flerovium",  289. } },
    { "Mc", {115, "Moscovium",  289. } },
    { "Lv", {116, "Livermorium", 293. } },
    { "Ts", {117, "Tennessine", 293. } },
    { "Og", {118, "Oganesson",  294. } }
};




void SpeciesData::modifyDefaults(ParameterSet &ps)
{
    auto& p = ps.get<SelectableSubsetParameter>("properties");
    ParameterSet fl( p.getParametersForSelection("fromLibrary") );
    auto& sl = fl.get<SelectionParameter>("specie");

    if (speciesLibrary_.size()>0)
    {
        SelectionParameter::ItemList ni;
        std::transform(speciesLibrary_.begin(), speciesLibrary_.end(),
                       std::back_inserter(ni),
                       [](const SpeciesLibrary::value_type& sle)
                       {
                           return sle.first;
                       }
                       );
        sl.resetItems(ni);
        if (speciesLibrary_.count("N2"))
            sl.setSelection("N2");
    }
    p.setParametersForSelection("fromLibrary", fl);
}




SpeciesData::SpeciesData(const ParameterSet &ps)
{
    Parameters p(ps);

    // name_=p.name;

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
    // const std::string& name,
    const SpeciesData::SpeciesMixture& mixture
    )
    // : name_(name)
{
    Parameters::properties_custom_type::thermo_janaf_type mixthermo;
    std::fill(mixthermo.coeffs_hi.begin(), mixthermo.coeffs_hi.end(), 0.);
    std::fill(mixthermo.coeffs_lo.begin(), mixthermo.coeffs_lo.end(), 0.);

    Parameters::properties_custom_type::transport_sutherland_type mixtrans;
    double mix_st_A=0.0;
    mixtrans.Tref=0.0;

    Parameters::properties_custom_type::elements_type elements;

    double wtotal=0.0, Mq=0.;
    for (auto s=mixture.begin(); s!=mixture.end(); ++s)
    {
        Mq+=s->first / s->second.M();
    }
    Mq=1./Mq;

    for (auto s=mixture.begin(); s!=mixture.end(); ++s)
    {
        auto sp=s->second.p_;

        double wi = s->first;
        wtotal += wi;
        double xi = wi*Mq/s->second.M();


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
                mixthermo.coeffs_lo[i] += wi * jt->coeffs_lo[i];
                mixthermo.coeffs_hi[i] += wi * jt->coeffs_hi[i];
            }
        }
        else
        {
            throw insight::Exception("Can only lump mixtures which homogeneously use janaf polynomials.");
        }

        if (const auto *st = boost::get<Parameters::properties_custom_type::transport_sutherland_type>(&sp.transport))
        {
            mix_st_A += wi * sutherland_As(st->mu, st->Tref);
            mixtrans.Tref += wi * st->Tref;
        }
        else
        {
            throw insight::Exception("Can only lump mixtures which homogeneously use sutherland transport.");
        }

        for (const auto& e: sp.elements)
        {
            double nj=e.number*xi;

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


double SpeciesData::density(double T, double p) const
{
    const double RRjoule = 8314.51; // kJ/kg-mol-K

    if (boost::get<Parameters::properties_custom_type::equationOfState_perfectGas_type>(
            &p_.equationOfState))
    {
        double MM=M();
        return p/(RRjoule/MM)/T;
    }
    else
        throw insight::Exception("option not implemented");

    return 1.;
}

double SpeciesData::cp(double T, double p) const
{
    auto evalPoly = [](double T, const arma::mat& a)
    {
        return ((((a[4]*T + a[3])*T + a[2])*T + a[1])*T + a[0]);
    };

    if (const auto *ct = boost::get<Parameters::properties_custom_type::thermo_constant_type>(&p_.thermo))
    {
        return ct->Cp;
    }
    else if (const auto *jt = boost::get<Parameters::properties_custom_type::thermo_janaf_type>(&p_.thermo))
    {
        if (T<jt->Tmid)
            return evalPoly(T, jt->coeffs_lo);
        else
            return evalPoly(T, jt->coeffs_hi);
    }
    else
        throw insight::Exception("cp computation not implemented");
    return 0.;
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




std::string SpeciesData::equationOfStateType() const
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
    else if (boost::get<Parameters::properties_custom_type::equationOfState_PengRobinson_type>(
                 &p_.equationOfState))
    {
        return "PengRobinsonGas";
    }
    else if (boost::get<Parameters::properties_custom_type::equationOfState_rhoConst_type>(
                 &p_.equationOfState))
    {
        return "rhoConst";
    }
    else throw UnhandledSelection();

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
    OFDictData::dict eos;
    if (boost::get<Parameters::properties_custom_type::equationOfState_perfectGas_type>(
            &p_.equationOfState))
    {
    }
    else if (const auto * pf =
             boost::get<Parameters::properties_custom_type::equationOfState_perfectFluid_type>(
                 &p_.equationOfState))
    {
        eos["R"]=pf->R;
        eos["rho0"]=pf->rho0;
    }
    else if (const auto * apf =
             boost::get<Parameters::properties_custom_type::equationOfState_adiabaticPerfectFluid_type>(
                 &p_.equationOfState))
    {
        eos["p0"]=apf->p0;
        eos["rho0"]=apf->rho0;
        eos["B"]=apf->B;
        eos["gamma"]=apf->gamma;
    }
    else if (const auto * rp =
             boost::get<Parameters::properties_custom_type::equationOfState_rPolynomial_type>(
                 &p_.equationOfState))
    {
        eos["C"]=OFDictData::list(rp->C);
    }
    else if (const auto * pre =
             boost::get<Parameters::properties_custom_type::equationOfState_PengRobinson_type>(
                 &p_.equationOfState))
    {
        eos["Tc"]=pre->Tc;
        eos["Vc"]=0.0;
        eos["Pc"]=pre->Pc;
        eos["omega"]=pre->omega;
    }
    else if (const auto * rc =
             boost::get<Parameters::properties_custom_type::equationOfState_rhoConst_type>(
                 &p_.equationOfState))
    {
        eos["rho"]=rc->rho;
    }
    else throw UnhandledSelection();

    d["equationOfState"]=eos;
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
                            CurrentExceptionContext ex(2, "reading species data base "+itr->path().string());
                            std::ifstream fs(itr->path().string());
                            OFDictData::dict sd;
                            readOpenFOAMDict(fs, sd);

                            for (const auto& s: sd)
                            {
                                CurrentExceptionContext ex2(3, "reading species "+s.first);
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




} // namespace insight
