#ifndef INSIGHT_UNSTEADYCOMPRESSIBLENUMERICS_H
#define INSIGHT_UNSTEADYCOMPRESSIBLENUMERICS_H

#include "openfoam/caseelements/numerics/fvnumerics.h"

#include "openfoam/caseelements/numerics/pimplesettings.h"

#include "unsteadycompressiblenumerics__unsteadyCompressibleNumerics__Parameters_headers.h"

namespace insight {


class unsteadyCompressibleNumerics
    : public FVNumerics
{

public:
#include "unsteadycompressiblenumerics__unsteadyCompressibleNumerics__Parameters.h"

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

createGetters
<<<PARAMETERSET
*/


public:
    declareType ( "unsteadyCompressibleNumerics" );
    unsteadyCompressibleNumerics ( OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
    void addIntoDictionaries ( OFdicts& dictionaries ) const override;
    bool isCompressible() const override;
};


} // namespace insight

#endif // INSIGHT_UNSTEADYCOMPRESSIBLENUMERICS_H
