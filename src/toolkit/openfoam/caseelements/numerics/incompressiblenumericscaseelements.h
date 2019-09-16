#ifndef INCOMPRESSIBLENUMERICSCASEELEMENTS_H
#define INCOMPRESSIBLENUMERICSCASEELEMENTS_H

#include "openfoam/caseelements/basicnumericscaseelements.h"

namespace insight
{


class simpleFoamNumerics
    : public FVNumerics
{

public:
#include "incompressiblenumericscaseelements__simpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> simpleFoamNumerics Parameters
inherits FVNumerics::Parameters

checkResiduals = bool true "Whether to check residuals during run"
pinternal = double 0.0 "Internal pressure field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "simpleFoamNumerics" );
    simpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};




class pimpleFoamNumerics
    : public FVNumerics
{

public:
#include "incompressiblenumericscaseelements__pimpleFoamNumerics__Parameters.h"

/*
PARAMETERSET>>> pimpleFoamNumerics Parameters
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
    declareType ( "pimpleFoamNumerics" );
    pimpleFoamNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    virtual void addIntoDictionaries ( OFdicts& dictionaries ) const;
    static ParameterSet defaultParameters();
};

}

#endif // INCOMPRESSIBLENUMERICSCASEELEMENTS_H
