#ifndef INSIGHT_MAPPEDVELOCITYINLETBC_H
#define INSIGHT_MAPPEDVELOCITYINLETBC_H


#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

#include "mappedvelocityinletbc__MappedVelocityInletBC__Parameters_headers.h"

namespace insight {



class MappedVelocityInletBC
    : public BoundaryCondition
{
public:
#include "mappedvelocityinletbc__MappedVelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> MappedVelocityInletBC Parameters
inherits BoundaryCondition::Parameters

distance = vector (1 0 0) "distance of sampling plane"
average =  vector (1 0 0) "average"
rho = double 1025.0 "Density at boundary"
T = double 300.0 "Temperature at boundary"
gamma = double 1.0 "Ratio of specific heats at boundary"
phiName = string "phi" "Name of flux field"
psiName = string "none" "Name of compressibility field"
rhoName = string "none" "Name of density field"
UName = string "U" "Name of velocity field"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

createGetter
<<<PARAMETERSET
*/


public:
    declareType ( "MappedVelocityInletBC" );
    MappedVelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        ParameterSetInput ip = ParameterSetInput()
    );

    void addOptionsToBoundaryDict ( OFDictData::dict& bndDict ) const override;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};



} // namespace insight

#endif // INSIGHT_MAPPEDVELOCITYINLETBC_H
