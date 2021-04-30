#ifndef INSIGHT_VELOCITYINLETBC_H
#define INSIGHT_VELOCITYINLETBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/fielddata.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

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

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("VelocityInletBC");

    VelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );

    virtual void setField_p ( OFDictData::dict& BC, OFdicts& dictionaries, bool isPrgh  ) const;
    virtual void setField_U ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};



} // namespace insight

#endif // INSIGHT_VELOCITYINLETBC_H
