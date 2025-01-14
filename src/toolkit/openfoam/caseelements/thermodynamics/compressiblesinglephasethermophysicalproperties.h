#ifndef COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H
#define COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H

#include "openfoam/caseelements/basic/thermodynamicmodel.h"
#include "openfoam/caseelements/thermodynamics/speciesdata.h"

#include "compressiblesinglephasethermophysicalproperties__compressibleSinglePhaseThermophysicalProperties__Parameters_headers.h"


namespace insight {




class compressibleSinglePhaseThermophysicalProperties
    : public thermodynamicModel
{

    static void modifyDefaults(ParameterSet& ps);

public:
#include "compressiblesinglephasethermophysicalproperties__compressibleSinglePhaseThermophysicalProperties__Parameters.h"
/*
PARAMETERSET>>> compressibleSinglePhaseThermophysicalProperties Parameters
inherits thermodynamicModel::Parameters

addTo_makeDefault { modifyDefaults(p); }

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

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "compressibleSinglePhaseThermophysicalProperties" );
    compressibleSinglePhaseThermophysicalProperties ( OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );

    std::string requiredThermoType() const;
    std::unique_ptr<SpeciesData> speciesData() const;

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;

    static std::string category() { return "Material Properties"; }
};




} // namespace insight

#endif // COMPRESSIBLESINGLEPHASETHERMOPHYSICALPROPERTIES_H
