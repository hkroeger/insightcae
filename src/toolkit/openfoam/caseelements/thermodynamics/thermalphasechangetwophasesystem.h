#ifndef THERMALPHASECHANGETWOPHASESYSTEM_H
#define THERMALPHASECHANGETWOPHASESYSTEM_H

#include "openfoam/caseelements/thermophysicalcaseelements.h"

#include "openfoam/openfoamdict.h"
#include "thermalphasechangetwophasesystem_pdl.h"

namespace insight {

namespace thermalPhaseChangeTwoPhaseSystemModels {



class diameterModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_diameterModel
/*
PARAMETERSET>>>
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "diameterModel" );
    declareDynamicClass(diameterModel);

    diameterModel(ParameterSetInput ip = Parameters() );

    virtual void addIntoPhaseDict ( OFDictData::dict& phaseDict ) const =0;
};


class constantDiameter : public diameterModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_constantDiameter
/*
PARAMETERSET>>>
inherits diameterModel::Parameters

d =double  0.00045 "[m] constant diameter"

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "constant" );
    constantDiameter(ParameterSetInput ip = Parameters() );

    void addIntoPhaseDict ( OFDictData::dict& phaseDict ) const override;
};



class isothermalDiameter : public diameterModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_isothermalDiameter
/*
PARAMETERSET>>>
inherits diameterModel::Parameters

d0 =double  0.00045 "[m] reference diameter"
p0 =double  1e5 "[Pa] reference pressure"

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "isothermal" );
    isothermalDiameter(ParameterSetInput ip = Parameters() );

    void addIntoPhaseDict ( OFDictData::dict& phaseDict ) const override;
};




class phaseModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_phaseModel
/*
PARAMETERSET>>>

diameter =  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::diameterModel"
        default "constant"
        "The properties of the diameter model"

Sct=double 0.7 "turbulent Schmidt number"

residualAlpha = double 1e-4 ""

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "phaseModel" );
    declareDynamicClass(phaseModel);

    phaseModel(ParameterSetInput ip = Parameters() );

    virtual void addIntoPhaseDict ( OFDictData::dict& phaseDict ) const;
};


class purePhase : public phaseModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_purePhase
/*
PARAMETERSET>>>
inherits phaseModel::Parameters

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "purePhase" );
    purePhase(ParameterSetInput ip = Parameters() );

    void addIntoPhaseDict ( OFDictData::dict& phaseDict ) const override;
};


class dragModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_dragModel
/*
PARAMETERSET>>>

residualRe = double 1e-3 ""

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "dragModel" );
    declareDynamicClass(dragModel);

    dragModel(ParameterSetInput ip = Parameters() );

    virtual OFDictData::dict dragModelDict() const =0;
};




class SchillerNaumannDrag : public dragModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_SchillerNaumannDrag
/*
PARAMETERSET>>>
inherits dragModel::Parameters

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "SchillerNaumann" );
    SchillerNaumannDrag(ParameterSetInput ip = Parameters() );

    OFDictData::dict dragModelDict() const override;
};


class blendingModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_blendingModel
/*
PARAMETERSET>>>
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "blendingModel" );
    declareDynamicClass(blendingModel);

    blendingModel(ParameterSetInput ip = Parameters() );

    virtual OFDictData::dict blendingProperties() const =0;
};



class noBlending : public blendingModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_noBlending
/*
PARAMETERSET>>>
inherits blendingModel::Parameters

continuousPhase = labeledArrayKeySelection "../../../phases" ""
  "The name of the phase, which is not dispersed. The dispersed phases are the others."

createGetter
<<<PARAMETERSET
*/
    declareType ( "none" );
    noBlending(ParameterSetInput ip = Parameters() );

    OFDictData::dict blendingProperties() const override;
};



class saturationModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_saturationModel
/*
PARAMETERSET>>>
createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "saturationModel" );
    declareDynamicClass(saturationModel);

    saturationModel(ParameterSetInput ip = Parameters() );

    virtual OFDictData::dict saturationModelDict() const =0;
};



class TabulatedSaturation : public saturationModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_TabulatedSaturation
/*
PARAMETERSET>>>
inherits saturationModel::Parameters


file = path "" "path to CSV file containing a table of saturation temperature vs. pressure"

nHeaderLine         1;
refColumn           0;
#componentColumns    ;
separator           =string "," ""
mergeSeparators     = bool no ""

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "CSVtabulated" );
    TabulatedSaturation(ParameterSetInput ip = Parameters() );

    OFDictData::dict saturationModelDict() const override;
};



class heatTransferModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_heatTransferModel
/*
PARAMETERSET>>>

residualAlpha = double 1e-3 "residual phase fraction"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "heatTransferModel" );
    declareDynamicClass(heatTransferModel);

    heatTransferModel(ParameterSetInput ip = Parameters() );

    virtual OFDictData::dict heatTransferModelDict() const =0;
};


class sphericalHeatTransfer : public heatTransferModel
{

public:
    declareType ( "spherical" );
    sphericalHeatTransfer(ParameterSetInput ip = Parameters() );

    OFDictData::dict heatTransferModelDict() const override;
};


class turbulentDispersionModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_turbulentDispersionModel
/*
PARAMETERSET>>>
residualAlpha = double 1e-3 "residual phase fraction"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "turbulentDispersionModel" );
    declareDynamicClass(turbulentDispersionModel);

    turbulentDispersionModel(ParameterSetInput ip = Parameters() );

    virtual OFDictData::dict turbulentDispersionModelDict() const =0;
};

class constantCoefficient : public turbulentDispersionModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_constantCoefficient
/*
PARAMETERSET>>>
inherits turbulentDispersionModel::Parameters

Ctd = double 0.1 "dispersion coefficient"

createGetter
<<<PARAMETERSET
*/
public:
    declareType ( "constantCoefficient" );
    constantCoefficient(ParameterSetInput ip = Parameters() );

    OFDictData::dict turbulentDispersionModelDict() const override;
};

}



class thermalPhaseChangeTwoPhaseSystem
    : public thermodynamicModel
{
public:
#include THERMALPHASECHANGETWOPHASESYSTEM_PDL_thermalPhaseChangeTwoPhaseSystem
/*
PARAMETERSET>>>
inherits OpenFOAMCaseElement::Parameters

description
"This case elements is yet a stub"

phaseChange = bool true "Whether the phase change is allowed"


phases = labeledarray "phase%d" [
  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::phaseModel"
        default "purePhase"
        "The properties of the phase of the system"
] *1 "phases in the system"


drag = labeledarray keysFrom "phases" [ set {
 inPhase = labeledArrayKeySelection "../../phases" "" "the phase in which the drag is to be computed"
 model =  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::dragModel"
        default "SchillerNaumann"
        "The properties of the drag model"
} ] *0 "drag models"

blending = labeledarray "blending%d" [
  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::blendingModel"
        default "none"
        "The properties of the phase blending"
] *1 "blending models. This defines which phase is dispersed."

heatTransfer = labeledarray keysFrom "phases" [ set {
 inPhase = labeledArrayKeySelection "../../phases" "" "the phase in which the heat transfer is to be computed"
 model =  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::heatTransferModel"
        default "spherical"
        "The properties of the heat transfer model model"
} ] *0 "heat transfer models"

turbulentDispersion = labeledarray keysFrom "phases" [ set {
 inPhase = labeledArrayKeySelection "../../phases" "" "the phase in which the turbulent dispersion is to be computed"
 model =  dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::turbulentDispersionModel"
        default "constantCoefficient"
        "The properties of the turbulent dispersion model"
} ] *0 "turbulent dispersion models"

pMin = double 1e4 "[Pa] Minimum pressure"

saturationModel = dynamicclassconfig "thermalPhaseChangeTwoPhaseSystemModels::saturationModel"
        default "CSVtabulated"
        "The properties of the saturation curve"

createGetter
<<<PARAMETERSET
*/


public:
    declareType("thermalPhaseChangeTwoPhaseSystem");
    thermalPhaseChangeTwoPhaseSystem(OpenFOAMCase &c, ParameterSetInput ip = Parameters());
    void addIntoDictionaries(OFdicts &dictionaries) const override;
    virtual bool isUnique() const;

};

} // namespace insight

#endif
