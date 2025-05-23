#ifndef INSIGHT_VELOCITYINLETBC_H
#define INSIGHT_VELOCITYINLETBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/fielddata.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

#include "velocityinletbc__VelocityInletBC__Parameters_headers.h"

namespace insight {


/**
 * The base class for inlet boundaries in OpenFOAM.
 * It handles all informations for the creation of a dirichlet BC in the velocity field (mean velocity in turbulent cases).
 * No handling of turbulence at this stage.
 */
class VelocityInletBC
    : public BoundaryCondition
{
public:
#include "velocityinletbc__VelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> VelocityInletBC Parameters
inherits BoundaryCondition::Parameters

velocity = includedset "FieldData::Parameters" "Velocity specification"

T = includedset "FieldData::Parameters" "Temperature at boundary"
   modifyDefaults {
    selectablesubset fielddata = uniformSteady;
    vector fielddata/value = 300.0;
   }

rho = includedset "FieldData::Parameters" "Density at boundary"
   modifyDefaults {
    selectablesubset fielddata = uniformSteady;
    vector fielddata/value = 1.0;
   }

turbulence = dynamicclassconfig "turbulenceBC::turbulenceBC" default "uniformIntensityAndLengthScale" "Definition of the turbulence state at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

VoFWave =selectablesubset {{

 none set {}

 enabled set {
   waveType = selection (stokesFirst) stokesFirst
   Tsoft = double 0 "ramp-up time"
   depth = double 1 "depth of the wave"
   period = double 10 "wave period"
   direction = vector (1 0 0) "wave direction vector"
   phi = double 0 "wave phase shift"
   height = double 1 "wave height"

   relaxationZone = selectablesubset {{

    patchDist set {
     width = double 1 "width of the relaxation zone"
    }

   }} patchDist ""
 }

}} none ""

createGetter
<<<PARAMETERSET
*/

public:
    declareType("VelocityInletBC");

    VelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters()
    );

    virtual void setField_p ( OFDictData::dict& BC, OFdicts& dictionaries, bool isPrgh  ) const;
    virtual void setField_U ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    void modifyCaseOnDisk ( const OpenFOAMCase& cm, const boost::filesystem::path& location ) const override;

};



} // namespace insight

#endif // INSIGHT_VELOCITYINLETBC_H
