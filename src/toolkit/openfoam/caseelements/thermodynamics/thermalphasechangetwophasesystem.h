#ifndef THERMALPHASECHANGETWOPHASESYSTEM_H
#define THERMALPHASECHANGETWOPHASESYSTEM_H

#include "openfoam/caseelements/thermophysicalcaseelements.h"

#include "thermalphasechangetwophasesystem__thermalPhaseChangeTwoPhaseSystem__Parameters_headers.h"

namespace insight {

class thermalPhaseChangeTwoPhaseSystem
    : public thermodynamicModel
{
public:
#include "thermalphasechangetwophasesystem__thermalPhaseChangeTwoPhaseSystem__Parameters.h"
/*
PARAMETERSET>>> thermalPhaseChangeTwoPhaseSystem Parameters
inherits OpenFOAMCaseElement::Parameters

description
"This case elements is yet a stub"

phaseChange = bool true "Whether the phase change is allowed"

phases = labeledarray "phase%d" [
  set { }
] *1 "phases in the system"

drag = labeledarray keysFrom "phases" [ set {
 inPhase = labeledArrayKeySelection "../../phases" "" "the phase in which the drag is to be computed"
} ] *0 "drag models"

createGetter
<<<PARAMETERSET
*/

    /*

phases = labeledarray "phase%d" [
  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystem::PhaseModel" default "thermalPhaseChangeTwoPhaseSystem::purePhase"
        "The properties of the phase of the system"
] *1 "phases in the system"

blending = array [
  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystem::blendingModel" default "thermalPhaseChangeTwoPhaseSystem::noBlending"
        "The properties of the phase of the system"
] *1 "blending models"

drag = labeledarray keysFrom phases [ set {
 inPhase = selection ()
 model =  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystem::dragModel" default "thermalPhaseChangeTwoPhaseSystem::SchillerNaumannDrag"
        "The properties of the drag model"
} ] *0 "drag models"


saturationModel = dynamicclassconfig "saturationModel" default "CSV"
            "The saturation curve"
*/


public:
    declareType("thermalPhaseChangeTwoPhaseSystem");
    thermalPhaseChangeTwoPhaseSystem(OpenFOAMCase &c, ParameterSetInput ip = Parameters());
    void addIntoDictionaries(OFdicts &dictionaries) const override;
    virtual bool isUnique() const;

};

} // namespace insight

#endif
