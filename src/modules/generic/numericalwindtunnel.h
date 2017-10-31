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
#ifndef INSIGHT_NUMERICALWINDTUNNEL_H
#define INSIGHT_NUMERICALWINDTUNNEL_H

#include "openfoam/openfoamanalysis.h"
#include "openfoam/openfoamcaseelements.h"
#include "cadfeatures.h"

namespace insight {

class NumericalWindtunnel 
: public OpenFOAMAnalysis
{
public:
#include "numericalwindtunnel__NumericalWindtunnel__Parameters.h"
/*
PARAMETERSET>>> NumericalWindtunnel Parameters
inherits OpenFOAMAnalysis::Parameters

geometry = set {

 LupstreamByL   = double 5 "[-] upstream domain extent"
 LdownstreamByL = double 5 "[-] downstream domain extent"
 LasideByL      = double 5 "[-] lateral domain extent"
 LupByL         = double 5 "[-] height of the domain (above floor)"
 forwarddir     = vector (0 -1 0) "direction from rear to forward end in CAD geometry CS"
 upwarddir      = vector (0 0 1) "vertical direction in CAD geometry CS"
 
 objectfile     = path "object.stp" "path to object geometry"
 
} "Geometrical properties of the domain"
      


mesh = set {

 nx             = int 30 "# cells across object length"
 boxlevel       = int 2 "refinement level around object"
 rearlevel      = int 3 "refinement level around rear end of object"
 lmsurf         = int 2 "# cells across object length"
 lxsurf         = int 4 "# cells across object length"
 nlayer         = int 4 "# prism layers"
 tlayer         = double 0.5 "final layer ratio"
 grad_upstream  = double 10 "grading towards center"
 grad_downstream = double 10 "grading towards center"
 grad_aside     = double 10 "grading towards center"
 grad_up        = double 10 "grading towards center"
 
} "Properties of the computational mesh"



operation = set {

 v              = double 5.0 "[m/s] incident velocity"
 
} "Definition of the operation point under consideration"
      


fluid = set {

 rho            = double 1.0 "[kg/m^3] Density of the fluid"
 nu             = double 1.5e-5 "[m^2/s] Viscosity of the fluid"
 
} "Parameters of the fluid"

<<<PARAMETERSET
*/
  
protected:
  cad::FeaturePtr object_;
  arma::mat translation_;
  double L_, w_, h_;
  boost::filesystem::path objectSTLFile_;
  
public:
  declareType("Car Resistance");
  
  NumericalWindtunnel(const ParameterSet& ps, const boost::filesystem::path& exepath);
  
  static ParameterSet defaultParameters();
  static std::string category() { return "Generic Analyses"; }
  
  virtual void calcDerivedInputData();
  
  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);
  
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
};


}

#endif
