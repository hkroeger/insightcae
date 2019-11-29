#ifndef INSIGHT_PRESSUREOUTLETBC_H
#define INSIGHT_PRESSUREOUTLETBC_H


#include "openfoam/caseelements/boundarycondition.h"

#include "openfoam/fielddata.h"
#include "openfoam/caseelements/boundaryconditions/boundarycondition_multiphase.h"

namespace insight {


class PressureOutletBC
    : public BoundaryCondition
{
public:
#include "pressureoutletbc__PressureOutletBC__Parameters.h"
/*
PARAMETERSET>>> PressureOutletBC Parameters

prohibitInflow = bool true "Whether to clip velocities to zero in case of flow reversal"

behaviour = selectablesubset {{

 uniform
 set {
  pressure = includedset "FieldData::Parameters" "Pressure values at the boundary"
   modifyDefaults {
    selectablesubset fielddata = uniformSteady;
    vector fielddata/value = 0.0;
   }
 }

 fixMeanValue
 set {
  pressure = double 0.0 "Uniform static pressure at selected boundary patch"
 }

 timeVaryingUniform
 set {
  sequel = array [ set {
   time = double 0.0 "Simulation time"
   pressure = double 0.0 "Pressure value"
  } ] *1 "Sequel of pressure values. Values in between will be interpolated. Values at minimum and maximum time will be used beyond interval."
 }


 waveTransmissive
 set {
  pressure = double 0.0 "Uniform static pressure at selected boundary patch"
  kappa = double 1.4 "Specific heat ratio"
  L = double 1 "Reference length"
  rhoName = string "none" "rho field name"
  psiName = string "thermo:psi" "psi field name"
 }

 removePRGHHydrostaticPressure
 set {
  pressure = double 0.0 "Uniform static pressure at selected boundary patch"
 }

}} uniform "Behaviour of the pressure BC"

rho = double 1025.0 "Density"
phasefractions = dynamicclassconfig "multiphaseBC::multiphaseBC" default "uniformPhases" "Definition of the multiphase mixture composition"

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_;
    boost::filesystem::path casedir_;

public:
    declareType ( "PressureOutletBC" );
    PressureOutletBC
    (
        OpenFOAMCase& c,
        const std::string& patchName,
        const OFDictData::dict& boundaryDict,
        const ParameterSet& ps = Parameters::makeDefault(),
        const boost::filesystem::path& casedir = "."
    );
    void addIntoFieldDictionaries ( OFdicts& dictionaries ) const override;

};



} // namespace insight

#endif // INSIGHT_PRESSUREOUTLETBC_H
