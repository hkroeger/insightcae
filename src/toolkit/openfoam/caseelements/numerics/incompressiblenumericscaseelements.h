#ifndef INCOMPRESSIBLENUMERICSCASEELEMENTS_H
#define INCOMPRESSIBLENUMERICSCASEELEMENTS_H

#include "openfoam/caseelements/numerics/basicnumericscaseelements.h"

namespace insight
{




class steadyIncompressibleNumerics
    : public FVNumerics
{

public:
#include "incompressiblenumericscaseelements__steadyIncompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> steadyIncompressibleNumerics Parameters
inherits FVNumerics::Parameters

checkResiduals = bool true "Whether to check residuals during run"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "steadyIncompressibleNumerics" );
    steadyIncompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault(), const std::string& pName="p" );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};




class unsteadyIncompressibleNumerics
    : public FVNumerics
{

public:
#include "incompressiblenumericscaseelements__unsteadyIncompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> unsteadyIncompressibleNumerics Parameters
inherits FVNumerics::Parameters

time_integration = includedset "insight::PIMPLESettings::Parameters" "Settings for time integration"
  modifyDefaults {
     selectablesubset timestep_control = adjust;
     double timestep_control/maxCo = 5.0;
  }

forceLES = bool false "Whether to enforce LES numerics"
LESfilteredConvection = bool false "Whether to use filtered linear convection schemes instead of linear when using LES"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "unsteadyIncompressibleNumerics" );
    unsteadyIncompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault(), const std::string& pName="p" );

    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;

};

}

#endif // INCOMPRESSIBLENUMERICSCASEELEMENTS_H
