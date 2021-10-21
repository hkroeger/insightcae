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
#include "parametersetvisualizer.h"
#include "numericalwindtunnel__NumericalWindtunnel__Parameters_headers.h"

#include "gp_Trsf.hxx"

namespace insight {


class NumericalWindtunnel_ParameterSet_Visualizer;

class NumericalWindtunnel 
: public OpenFOAMAnalysis
{
  friend class NumericalWindtunnel_ParameterSet_Visualizer;

public:
  static void modifyDefaults(ParameterSet& p);
#include "numericalwindtunnel__NumericalWindtunnel__Parameters.h"
/*
PARAMETERSET>>> NumericalWindtunnel Parameters
inherits OpenFOAMAnalysis::Parameters
addTo_makeDefault { modifyDefaults(p); }

geometry = set {

 LupstreamByL   = double 4 "[-] upstream domain extent, divided by object diagonal" *hidden
 LdownstreamByL = double 10 "[-] downstream domain extent, divided by object diagonal" *hidden
 LasideByL      = double 3 "[-] lateral domain extent, divided by object diagonal" *hidden
 LupByL         = double 3 "[-] height of the domain (above floor), divided by object diagonal" *hidden
 forwarddir     = vector (1 0 0) "direction from rear to forward end in CAD geometry CS"
 upwarddir      = vector (0 0 1) "vertical direction in CAD geometry CS"
 
 objectfile     = path "object.stp" "Path to object geometry. May be STL, STEP or IGES." *necessary
 
} "Geometrical properties of the domain"
      
geometryscale = double 1e-3     "scaling factor to scale geometry files to meters"

mesh = set {

 nx             = int 10 "number of cells across object diagonal" *necessary
 boxlevel       = int 2 "refinement level in bounding box around object"
 rearlevel      = int 3 "refinement level in wake of object"
 lmsurf         = int 2 "minimum refinement level on object surface" *necessary
 lxsurf         = int 4 "maximum refinement level on object surface" *necessary
 nlayer         = int 4 "number of prism layers"
 tlayer         = double 0.5 "final layer ratio" *hidden
 grad_upstream  = double 10 "mesh grading from object towards inlet" *hidden
 grad_downstream = double 10 "mesh grading from object towards outlet" *hidden
 grad_aside     = double 10 "mesh grading from object towards lateral boundary" *hidden
 grad_up        = double 10 "mesh grading from object towards upper boundary" *hidden

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

  struct supplementedInputData
      : public supplementedInputDataDerived<Parameters>
  {
  public:
    supplementedInputData(std::unique_ptr<Parameters> p,
                          const boost::filesystem::path& workDir,
                          ProgressDisplayer& progress = consoleProgressDisplayer );

    gp_Trsf cad_to_cfd_;
    double Lref_, l_, w_, h_;
  };

#ifndef SWIG
  defineBaseClassWithSupplementedInputData(Parameters, supplementedInputData)
#endif
  
public:
  declareType("Numerical Wind Tunnel");
  
  NumericalWindtunnel(const ParameterSet& ps, const boost::filesystem::path& exepath, ProgressDisplayer& pd);

  static std::string category() { return "Generic Analyses"; }
  
  void calcDerivedInputData(ProgressDisplayer& parentActionProgress) override;
  
  void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
  void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
  
  ParameterSet parameters() const override;

  ResultSetPtr evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
};




class NumericalWindtunnel_ParameterSet_Visualizer
 : public CAD_ParameterSet_Visualizer
{
public:
    typedef NumericalWindtunnel::Parameters Parameters;

public:
    void recreateVisualizationElements() override;
};

}

#endif
