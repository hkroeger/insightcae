#ifndef COMPRESSIBLENUMERICSCASEELEMENTS_H
#define COMPRESSIBLENUMERICSCASEELEMENTS_H

#include "openfoam/caseelements/numerics/basicnumericscaseelements.h"

namespace insight
{




class steadyCompressibleNumerics
    : public FVNumerics
{

public:
#include "compressiblenumericscaseelements__steadyCompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> steadyCompressibleNumerics Parameters
inherits FVNumerics::Parameters

nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"

pinternal = double 1e5 "Internal pressure field value"
Tinternal = double 300 "Internal temperature field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

rhoMin = double 0.01 "Lower clipping for density"
rhoMax = double 100. "Upper clipping for density"
transonic = bool true "Check for transsonic flow"
consistent = bool true "Check for SIMPLEC instead of plain SIMPLE"

setup = selection ( accurate stable ) accurate "Select accuratefor second order schemes. In case of stability problems revert to stable."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "steadyCompressibleNumerics" );
    steadyCompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
    static ParameterSet defaultParameters();
};





class unsteadyCompressibleNumerics
    : public FVNumerics
{

public:
#include "compressiblenumericscaseelements__unsteadyCompressibleNumerics__Parameters.h"

/*
PARAMETERSET>>> unsteadyCompressibleNumerics Parameters
inherits FVNumerics::Parameters

nNonOrthogonalCorrectors = int 0 "Number of non-orthogonal correctors"
time_integration = includedset "insight::CompressiblePIMPLESettings::Parameters" "Settings for time integration"
  modifyDefaults {
     selectablesubset timestep_control = adjust;
     double timestep_control/maxCo = 5.0;
  }

formulation = selection ( sonicFoam rhoPimpleFoam ) sonicFoam "Solver to use"

pinternal = double 1e5 "Internal pressure field value"
Tinternal = double 300 "Internal temperature field value"
Uinternal = vector (0 0 0) "Internal velocity field value"

TMin = double 100. "Lower clipping for temperature"
TMax = double 6000. "Upper clipping for temperature"
rhoMin = double 0.01 "Lower clipping for density"
rhoMax = double 100. "Upper clipping for density"
UMax = double 1000. "Upper clipping for velocity"

setup = selection ( accurate medium stable ) accurate "Select accuratefor second order schemes. In case of stability problems revert to stable."

<<<PARAMETERSET
*/

protected:
    Parameters p_;

public:
    declareType ( "unsteadyCompressibleNumerics" );
    unsteadyCompressibleNumerics ( OpenFOAMCase& c, const ParameterSet& ps = Parameters::makeDefault() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
    static ParameterSet defaultParameters();
};



}

#endif // COMPRESSIBLENUMERICSCASEELEMENTS_H
