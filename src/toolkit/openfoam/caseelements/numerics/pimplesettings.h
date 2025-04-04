#ifndef PIMPLESETTINGS_H
#define PIMPLESETTINGS_H


#include "base/parameterset.h"
#include "openfoam/openfoamcase.h"
#include "pimplesettings__PIMPLESettings__Parameters_headers.h"

namespace insight
{

class PIMPLESettings
{
public:
#include "pimplesettings__PIMPLESettings__Parameters.h"

/*
PARAMETERSET>>> PIMPLESettings Parameters

nonOrthogonalCorrectors = int 0 "Number of additional correction steps for mesh non-orthogonality."
momentumPredictor = bool true "Whether the velocity equation shall be solved with an implicit solver"

timestep_control = selectablesubset {{

 fixed set { }

 adjust set {
   maxCo = double 1.0 "Target courant number. For PISO, this should be lower than 1. For SIMPLE, values of 5 or larger should work."
   maxDeltaT = double 1.0 "Maximum time step size"
 }

}} adjust "Whether to allow time step adjustment during execution"


pressure_velocity_coupling = selectablesubset {{

 PISO set {
  correctors = int 2 "Number of correctors (2=2nd order, 3=third order, ...)"
 }

 SIMPLE set {
  max_nOuterCorrectors = int 25 "Maximum number of SIMPLE correctors. The outer iteration loop is terminated earlier, if prescribed target residuals are met."

  residual_p = double 0.001 "target pressure residual"
  residual_U = double 0.001 "target velocity residual"

  relaxation_p = double 0.2 "pressure relaxation factor"
  relaxation_U = double 0.5 "velocity relaxation factor"
  relaxation_e = double 0.5 "energy relaxation factor"
  relaxation_turb = double 0.7 "turbulence quantity relaxation factor"

  relax_final = bool false "whether to relax in the final outer iteration"
 }

 PIMPLE set {
  max_nOuterCorrectors = int 2 "Maximum number of SIMPLE correctors. The outer iteration loop is terminated earlier, if prescribed target residuals are met."
  nCorrectors = int 2 "Number of PISO correctors inside of one outer corrector step."

  residual_p = double 0.001 "target pressure residual"
  residual_U = double 0.001 "target velocity residual"

  relaxation_p = double 0.3 "pressure relaxation factor"
  relaxation_U = double 0.8 "velocity relaxation factor"
  relaxation_e = double 0.8 "energy relaxation factor"
  relaxation_turb = double 0.8 "turbulence quantity relaxation factor"

  relax_final = bool false "whether to relax in the final outer iteration"
 }

}} SIMPLE "Pressure-velocity coupling scheme"


pRefValue = double 0 "Reference pressure value"

pRefLocation = selectablesubset {{
 cellID set { pRefCell = int 0 "Reference pressure cell index" }
 location set { pRefPoint = vector (0 0 0) "Reference pressure point coordinates" }
}} cellID "Location of reference pressure"

createGetter
<<<PARAMETERSET
*/

public:
  PIMPLESettings(ParameterSetInput ip = Parameters() );
  virtual ~PIMPLESettings();

  virtual void addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const;

  bool isPISO() const;
  bool isSIMPLE() const;
};




class CompressiblePIMPLESettings
    : public PIMPLESettings
{
public:
#include "pimplesettings__CompressiblePIMPLESettings__Parameters.h"

/*
PARAMETERSET>>> CompressiblePIMPLESettings Parameters
inherits PIMPLESettings::Parameters

transonic = bool false "Set to true, if flow is transonic; false for subsonic flow."

relaxation_rho = double 1.0 "relaxation factor for density"

pMin = double 1000 "Minimum pressure"
rhoMin = double 0 "Minimum density"
rhoMax = double 1e10 "Maximum density"

createGetter
<<<PARAMETERSET
*/

public:
  CompressiblePIMPLESettings(ParameterSetInput ip = Parameters() );

  void addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const override;

};



class MultiphasePIMPLESettings
    : public PIMPLESettings
{
public:
#include "pimplesettings__MultiphasePIMPLESettings__Parameters.h"

/*
PARAMETERSET>>> MultiphasePIMPLESettings Parameters
inherits PIMPLESettings::Parameters

maxAlphaCo = double 0.5 "Target courant number at free surface"

createGetter
<<<PARAMETERSET
*/

public:
  MultiphasePIMPLESettings(ParameterSetInput ip = Parameters() );

  void addIntoDictionaries ( const OpenFOAMCase& oc, OFdicts& dictionaries ) const override;
};



}

#endif // PIMPLESETTINGS_H
