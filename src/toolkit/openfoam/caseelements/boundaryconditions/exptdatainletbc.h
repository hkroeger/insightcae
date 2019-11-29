#ifndef INSIGHT_EXPTDATAINLETBC_H
#define INSIGHT_EXPTDATAINLETBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

namespace insight {

namespace OFDictData { class dict; }

class ExptDataInletBC
    : public BoundaryCondition
{
public:
#include "exptdatainletbc__ExptDataInletBC__Parameters.h"
/*
PARAMETERSET>>> ExptDataInletBC Parameters

data = array[ set {
    point = vector (0 0 0) "Point coordinate"
    velocity = vector (0 0 0) "Velocity at this point"
    k = double 0.1 "Turbulent kinetic energy at this point"
    epsilon = double 0.1 "Turbulent dissipation rate"
} ] *1 "Velocity specification per point"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;

public:
    declareType("ExptDataInletBC");

    ExptDataInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );

    virtual void addDataDict ( OFdicts& dictionaries, const std::string& prefix, const std::string& fieldname, const arma::mat& data ) const;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};




} // namespace insight

#endif // INSIGHT_EXPTDATAINLETBC_H
