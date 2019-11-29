#ifndef INSIGHT_WALLBC_H
#define INSIGHT_WALLBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_meshmotion.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_heat.h"


namespace insight {

namespace OFDictData { class dict; }

class WallBC
    : public BoundaryCondition
{
public:
#include "wallbc__WallBC__Parameters.h"
/*
PARAMETERSET>>> WallBC Parameters

wallVelocity = vector (0 0 0) "Velocity of the wall surface"
rotating = bool false "Whether the wall is rotating"
CofR = vector (0 0 0) "Center of rotation"
roughness_z0 = double 0 "Wall roughness height"
meshmotion = dynamicclassconfig "MeshMotionBC::MeshMotionBC" default "NoMeshMotion" "Mesh motion properties at the boundary"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"
heattransfer = dynamicclassconfig "HeatBC::HeatBC" default "Adiabatic" "Definition of the heat transfer through the wall"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("WallBC");

    WallBC
    (
        OpenFOAMCase& c, const std::string& patchName, const OFDictData::dict& boundaryDict,
        const ParameterSet &ps = Parameters::makeDefault()
    );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;
    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;


};


} // namespace insight

#endif // INSIGHT_WALLBC_H
