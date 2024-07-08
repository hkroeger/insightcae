#ifndef COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H
#define COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H

#include "openfoam/caseelements/basic/thermodynamicmodel.h"
#include "openfoam/caseelements/thermodynamics/speciesdata.h"

#include "compressiblesinglephasethermophysicalproperties__compressibleSinglePhaseThermophysicalProperties__Parameters_headers.h"


namespace insight {




class compressibleSinglePhaseThermophysicalProperties
    : public thermodynamicModel
{

public:
#include "compressiblesinglephasethermophysicalproperties__compressibleSinglePhaseThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleSinglePhaseThermophysicalProperties Parameters


composition = selectablesubset {{

  singleSpecie
    includedset "insight::SpeciesData::Parameters"

  staticSpeciesMixture set {
    components = array [ set {
      specie = includedset "insight::SpeciesData::Parameters"
      fraction = selectablesubset {{
        moleFraction set {
          value = double 1 "mole fraction of the specie"
        }
        massFraction set {
          value = double 1 "mass fraction of the specie"
        }
      }} massFraction "(constant) fraction of this specie in the mixture"
    } ] *1 "Species in the mixture"
  }

}} singleSpecie ""

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




} // namespace insight

#endif // COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H
