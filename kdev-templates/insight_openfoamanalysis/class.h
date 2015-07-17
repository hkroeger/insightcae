{% load kdev_filters %}
/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef {% include "include_guard_cpp.txt" %}
#define {% include "include_guard_cpp.txt" %}
 
#include <openfoam/openfoamanalysis.h>

namespace insight
{
  
class {{ name }} 
: insight::OpenFOAMAnalysis
{
  
public:
#include "{{ name|lower }}__{{ name }}__Parameters.h"
  
/*
PARAMETERSET>>> {{ name }} Parameters

geometry=set
{
 D 		= double 	0.05 		"[m] Diameter of outer pipe"
 d 		= double 	0.01 		"[m] Diameter of inner pipe"
 tnozzle	= double 	0.002 		"[m] Wall thickness of nozzle"
 LByD 		= double 	8.0 		"Domain length, divided by diameter of outer pipe"
} "Geometrical properties of the jet mixer"

      
mesh=set
{
 yplusco	= double	2 		"y+ at outer pipe wall"
 yplusj		= double	2 		"y+ at jet shear layer"
 nrco		= int		60		"# cells in radial direction along coflow radius"
 nrj		= int		15		"# cells in radial direction along inner pipe radius"
 deltaxplus	= double	100 		"axial cell length (normalized with shear layer properties at jet boundary)"
 deltacplus	= double	100 		"circumferential cell length at pipe wall (normalized with shear layer properties at jet boundary)"
 gradx		= double	10 		"grading towards outlet"
} "Properties of the computational mesh"
    
operation=set
{
 Ubco		= double 	0.228 		"[m/s] Coflow bulk velocity"
 Ubj		= double 	1.14 		"[m/s] Jet bulk velocity"
 
 Ubcoshape 	= selectablesubset 
 {{
  block
  set {
  } "constant velocity profile"
  
  cutpowerlaw
  set {
   exponent     = double        7.0		"reciprocal power law exponent"
  } "power law shape, clipped in jet area"
  
 }} cutpowerlaw "Shape of the bulk flow velocity"

 Ubshape 	= selectablesubset 
 {{
  block
  set {
  } "constant velocity profile"
  
  powerlaw
  set {
   exponent     = double        7.0		"reciprocal power law exponent"
  } "power law shape"
  
 }} powerlaw "Shape of the jet exit velocity"

 turbj 	= selectablesubset 
 {{
 
  uniformisotropic
  set {
   intensity 	= double 	0.05		"uniform turbulence intensity in jet"
  } "uniform and isotropic turbulence properties"
  
  profile
  set {
   tablefile    = path "profile.dat" "Path to file with tabulated profile (ascii file, cols: 1. radius [m], 2. Rrr, 3. Rphiphi, 4. Rxx)"
  } "tabulated profile"
  
 }} uniformisotropic "Turbulence specification in jet"
 
 turbco 	= selectablesubset 
 {{
 
  uniformisotropic
  set {
   intensity 	= double 	0.05		"uniform turbulence intensity in coflow"
  } "uniform and isotropic turbulence properties"
  
  profile
  set {
   tablefile    = path "profile.dat" "Path to file with tabulated profile (ascii file, cols: 1. radius [m], 2. Rrr, 3. Rphiphi, 4. Rxx)"
  } "tabulated profile"
  
 }} uniformisotropic "Turbulence specification in coflow"
 
 spottype=selection ( 
  hatSpot 
  gaussianSpot
  decayingTurbulenceSpot
  decayingTurbulenceVorton
  anisotropicVorton
  modalTurbulence
  ) anisotropicVorton "Type of inflow generator"

  
} "Definition of the operation point under consideration"
     
fluid=set
{
 rho		= double 	998.0 		"[kg/m^3] Density of the fluid"
 nu		= double 	1e-6 		"[m^2/s] Viscosity of the fluid"
} "Parameters of the fluid"

evaluation=set
{
 inittime	= double 	1.0 		"[T] length of grace period before averaging starts (as multiple of flow-through time)"
 meantime	= double 	10.0 		"[T] length of time period for averaging of velocity and RMS (as multiple of flow-through time)"
 mean2time	= double 	10.0 		"[T] length of time period for averaging of second order statistics (as multiple of flow-through time)"
 eval2		= bool 		true		"Whether to evaluate second order statistics"
} "Parameters of the result evaluation"

<<<PARAMETERSET
*/

protected:
  // derived data

public:

  declareType("{{ name }}");
  
  {{ name }}(const NoParameters&);
  
  virtual ParameterSet defaultParameters() const;
  virtual void calcDerivedInputData();
  
  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);
    
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cmp);

};

}
#endif // {% include "include_guard_cpp.txt" %}