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
#include "parametersetvisualizer.h"

namespace insight {


class NumericalWindtunnel_ParameterSet_Visualizer;

class NumericalWindtunnel 
: public OpenFOAMAnalysis
{
  friend class NumericalWindtunnel_ParameterSet_Visualizer;

public:
#include "numericalwindtunnel__NumericalWindtunnel__Parameters.h"
/*
PARAMETERSET>>> NumericalWindtunnel Parameters
inherits OpenFOAMAnalysis::Parameters

geometry = set {

 LupstreamByL   = double 4 "[-] upstream domain extent" *hidden
 LdownstreamByL = double 10 "[-] downstream domain extent" *hidden
 LasideByW      = double 3 "[-] lateral domain extent" *hidden
 LupByH         = double 3 "[-] height of the domain (above floor)" *hidden
 forwarddir     = vector (0 -1 0) "direction from rear to forward end in CAD geometry CS"
 upwarddir      = vector (0 0 1) "vertical direction in CAD geometry CS"
 
 objectfile     = path "object.stp" "Path to object geometry. May be STL, STEP or IGES." *necessary
 
} "Geometrical properties of the domain"
      
geometryscale = double 1e-3     "scaling factor to scale geometry files to meters"

mesh = set {

 nx             = int 10 "# cells across object length" *necessary
 boxlevel       = int 2 "refinement level around object"
 rearlevel      = int 3 "refinement level around rear end of object"
 lmsurf         = int 2 "# cells across object length" *necessary
 lxsurf         = int 4 "# cells across object length" *necessary
 nlayer         = int 4 "# prism layers"
 tlayer         = double 0.5 "final layer ratio" *hidden
 grad_upstream  = double 10 "grading towards center" *hidden
 grad_downstream = double 10 "grading towards center" *hidden
 grad_aside     = double 10 "grading towards center" *hidden
 grad_up        = double 10 "grading towards center" *hidden

 refinementZones = array [ set {
  lx = int 5 "Refinement level inside zones"
  geometry = selectablesubset {{

   box_centered set {
    pc = vector (0 0 0) " [m] Center point" *necessary
    L = double 1.0 "[m] Length of the box along x direction, centered around pc" *necessary
    W = double 1.0 "[m] Width of the box along y direction, centered around pc" *necessary
    H = double 1.0 "[m] Height of the box along z direction, centered around pc" *necessary
   }

   box set {
    pmin = vector (0 0 0) "[m] Minimum corner" *necessary
    pmax = vector (1 1 1) "[m] Maximum corner" *necessary
   }

  }} box_centered "Geometry of the refinement zone"
 } ] *0 "Local refinement zones"

 longitudinalSymmetry = bool false "Include only half of the object in the CFD model. Apply symmetry BC in the mid plane."
 
} "Properties of the computational mesh"



operation = set {

 v              = double 1.0 "[m/s] incident velocity" *necessary
 
} "Definition of the operation point under consideration"
      


fluid = set {

 rho            = double 1.0 "[kg/m^3] Density of the fluid"
 nu             = double 1.5e-5 "[m^2/s] Viscosity of the fluid"
 
} "Parameters of the fluid"

<<<PARAMETERSET
*/
  
protected:

  gp_Trsf cad_to_cfd_;
  double Lref_, l_, w_, h_;
  
public:
  declareType("Numerical Wind Tunnel");
  
  NumericalWindtunnel(const ParameterSet& ps, const boost::filesystem::path& exepath);

  static std::string category() { return "Generic Analyses"; }
  
  virtual void calcDerivedInputData();
  
  virtual void createCase(insight::OpenFOAMCase& cm);
  virtual void createMesh(insight::OpenFOAMCase& cm);
  
  virtual ResultSetPtr evaluateResults(OpenFOAMCase& cm);
};




class NumericalWindtunnel_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    typedef NumericalWindtunnel::Parameters Parameters;

public:
    void recreateVisualizationElements(UsageTracker* ut) override;
};

}

#endif
