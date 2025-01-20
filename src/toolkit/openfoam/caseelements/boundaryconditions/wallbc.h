#ifndef INSIGHT_WALLBC_H
#define INSIGHT_WALLBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"

#include "wallbc__WallBC__Parameters_headers.h"

namespace insight {

namespace OFDictData { class dict; }

class WallBC
    : public BoundaryCondition
{
public:
#include "wallbc__WallBC__Parameters.h"
/*
PARAMETERSET>>> WallBC Parameters
inherits BoundaryCondition::Parameters

wallVelocity = vector (0 0 0) "Velocity of the wall surface"
rotating = bool false "Whether the wall is rotating"
CofR = vector (0 0 0) "Center of rotation"
roughness_z0 = double 0 "Wall roughness height"

meshmotion = dynamicclassconfig "insight::MeshMotionBC::MeshMotionBC" default "NoMeshMotion" "Mesh motion properties at the boundary"
phasefractions = dynamicclassconfig "insight::multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"
heattransfer = dynamicclassconfig "insight::HeatBC::HeatBC" default "Adiabatic" "Definition of the heat transfer through the wall"

createGetters
<<<PARAMETERSET
*/


public:
    declareType("WallBC");

    WallBC
    (
        OpenFOAMCase& c, const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = Parameters()
    );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;


};


} // namespace insight

#endif // INSIGHT_WALLBC_H
