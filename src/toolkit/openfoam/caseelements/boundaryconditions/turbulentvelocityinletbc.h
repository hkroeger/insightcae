#ifndef INSIGHT_TURBULENTVELOCITYINLETBC_H
#define INSIGHT_TURBULENTVELOCITYINLETBC_H

#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/fielddata.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"


namespace insight {


namespace OFDictData { class dict; }


class TurbulentVelocityInletBC
    : public BoundaryCondition
{

public:

    static const std::vector<std::string> inflowGenerator_types;

#include "turbulentvelocityinletbc__TurbulentVelocityInletBC__Parameters.h"
/*
PARAMETERSET>>> TurbulentVelocityInletBC Parameters

umean = includedset "FieldData::Parameters" "Mean velocity specification"

turbulence = selectablesubset {{

uniformIntensityAndLengthScale
set {
    intensity = double 0.05 "Turbulence intensity as fraction of mean velocity"
    lengthScale = double 0.1 "[m] Turbulence length scale"
}

inflowGenerator
set {
    uniformConvection=bool false "Whether to use a uniform convection velocity instead of the local mean velocity"

    volexcess=double 2.0 "Volumetric overlapping of spots"

    type=selection (
    hatSpot
    gaussianSpot
    decayingTurbulenceSpot
    decayingTurbulenceVorton
    anisotropicVorton_Analytic
    anisotropicVorton_PseudoInv
    anisotropicVorton_NumOpt
    anisotropicVorton2
    combinedVorton
    modalTurbulence
    ) anisotropicVorton_PseudoInv "Type of inflow generator"

    R=includedset "FieldData::Parameters" "Reynolds stresses specification"
       modifyDefaults {
        selectablesubset fielddata = uniformSteady;
        vector fielddata/value = 1 0 0 1 0 1;
       }

    L=includedset "FieldData::Parameters" "Length scale specification"
       modifyDefaults {
        selectablesubset fielddata = uniformSteady;
        vector fielddata/value = 1.0 1.0 1.0;
       }
}

}} uniformIntensityAndLengthScale "Properties of turbulence"

phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/


protected:
    ParameterSet ps_;
    Parameters p_;

public:
    declareType ( "TurbulentVelocityInletBC" );
    TurbulentVelocityInletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& p = Parameters::makeDefault()
    );

    virtual void setField_p ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_U ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_k ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_epsilon ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_omega ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_nuTilda ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    virtual void setField_R ( OFDictData::dict& BC, OFdicts& dictionaries ) const;
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};


} // namespace insight

#endif // INSIGHT_TURBULENTVELOCITYINLETBC_H
