#ifndef INSIGHT_MASSFLOWBC_H
#define INSIGHT_MASSFLOWBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_turbulence.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

#include "massflowbc__MassflowBC__Parameters_headers.h"

namespace insight {


class MassflowBC
    : public BoundaryCondition
{
public:
#include "massflowbc__MassflowBC__Parameters.h"
/*
PARAMETERSET>>> MassflowBC Parameters
inherits BoundaryCondition::Parameters

flowrate = selectablesubset {{
 massflow set { value = double 1.0 "mass flow through boundary" }
 volumetric set { value = double 1.0 "volumetric flow through boundary" }
}} massflow "Specification of flow rate through boundary."

rho = double 1025.0 "Density at boundary"
T = double 300.0 "Temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "rho" "Name of density field"
UName = string "U" "Name of velocity field"
turbulence = dynamicclassconfig "turbulenceBC::turbulenceBC" default "uniformIntensityAndLengthScale" "Definition of the turbulence state at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "MassflowBC" );
    MassflowBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters()
    );
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};



} // namespace insight

#endif // INSIGHT_MASSFLOWBC_H
