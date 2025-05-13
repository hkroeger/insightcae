#ifndef SPECIESDATA_H
#define SPECIESDATA_H

#include "base/parameterset.h"
#include "openfoam/openfoamdict.h"
#include "base/propertylibrary.h"

#include "speciesdata__SpeciesData__Parameters_headers.h"




namespace insight {




double sutherland_As(double mu, double Ts);
double sutherland_mu(double As, double Ts);




class CustomSpecies
{

public:

#include "speciesdata__CustomSpecies__Parameters.h"
/*
PARAMETERSET>>> CustomSpecies Parameters

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


 polynomial
 set {
   muCoeffs = array [ double 1000 "coefficient" ] *1
        "[Pa.s] Dynamic viscosity polynomial coefficients, lowest order (0, i.e. constant) coefficient first."
   kappaCoeffs = array [ double 2000 "coefficient" ] *1
        "[W/m/K] Thermal conductivity polynomial coefficients, lowest order (0, i.e. constant) coefficient first."
 }

 WLF
 set {
   mu0 = double 1 "reference viscosity"
   Tr = double 319.954 "[K] reference temperature"
   C1 = double 10.86 "constant C1"
   C2 = double 141.804 "constant C2"
   Pr = double 1 "Prandtl number"
 }

}} constant "Transport properties"


equationOfState = selectablesubset {{

  perfectGas set {}

  perfectFluid set {
    R = double 3000 "R in $\\rho_0+p/(R T)$"
    rho0 = double 1027 "Reference density"
  }

  // from https://www.cfd-online.com/Forums/openfoam-solving/228725-how-use-tait-equaton-state-compressibleinterfoam.html
  adiabaticPerfectFluid set {
    rho0 = double 1025 ""
    p0 = double 1e5 ""
    gamma = double 7.15 ""
    B = double 0.304913e8 ""
  }

  rhoConst
  set {
    rho = double 1.25 "[kg/m^3] Density"
  }

  rPolynomial set {
    C = array [ double 1 "" ] *0 "Polynomial coefficients"
  }

  icoPolynomial set {
   rhoCoeffs = array [ double 1000 "coefficient" ] *1
        "[kg/m^3] Density vs. temperature polynomial coefficients, lowest order (0, i.e. constant) coefficient first."
  }


  PengRobinson
  set {
   Tc = double 617. "[K] Critical temperature"
   Pc = double 3622400.0 "[Pa] Critical pressure"
   omega = double 0.304 "Acentric factor"
  }

}} perfectGas "Equation of state type"



elements = array [ set {
 element = string "N" "Name of the element"
 number = double 2 "Number of atoms per molecule. May be a fraction, if the specie represents a mixture."
} ] *1 "Elemental composition"


<<<PARAMETERSET
*/

};




class SpeciesLibrary
    : public MapPropertyLibrary<insight::CustomSpecies::Parameters>
{
    SpeciesLibrary();

public:
    static const SpeciesLibrary& library();
};





class SpeciesData
{

public:

    struct ElementData {
        int id;
        std::string name;
        double M;
    };

    typedef std::vector<std::pair<double, SpeciesData> > SpeciesMixture;

    // from https://www.qmul.ac.uk/sbcs/iupac/AtWt/
    static const std::map<std::string, SpeciesData::ElementData> elements;


#include "speciesdata__SpeciesData__Parameters.h"

/*
PARAMETERSET>>> SpeciesData Parameters

#name = string "N2" "Name of specie (i.e. the field name). Has to be unique!" *necessary

properties = selectablesubset {{

  fromLibrary set {
   #specie = selection ( no_library_available ) no_library_available "Species library entry"
   specie = librarySelection "insight::SpeciesLibrary" N2 "Species library entry"
  }

  custom includedset "insight::CustomSpecies::Parameters"

}} fromLibrary "Data of the specie."

<<<PARAMETERSET
*/

protected:
    insight::CustomSpecies::Parameters p_;


public:
    SpeciesData(const Parameters& p);
    SpeciesData(const SpeciesMixture& mixture);

    double M() const;

    double density(double T, double p) const;
    double cp(double T, double p) const;

    std::string transportType() const;
    std::string thermoType() const;
    std::string equationOfStateType() const;

    void insertSpecieEntries(OFDictData::dict& d) const;
    void insertThermodynamicsEntries(OFDictData::dict& d) const;
    void insertTransportEntries(OFDictData::dict& d) const;
    void insertEquationOfStateEntries(OFDictData::dict& d) const;
    void insertElementsEntries(OFDictData::dict& d) const;

    static SpeciesData::Parameters::properties_custom_type specieFromLibrary(const std::string& name);

    std::pair<double,double> temperatureLimits() const;
};




} // namespace insight

#endif // SPECIESDATA_H
