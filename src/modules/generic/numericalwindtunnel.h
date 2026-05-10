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


#include <map>

#include "base/parameters/subsetparameter.h"
#include "openfoam/openfoamanalysis.h"

#include "numericalwindtunnel__NumericalWindtunnel__Parameters_headers.h"

#include "gp_Trsf.hxx"

namespace insight {

namespace bmd { class blockMeshBlocking; }

class NumericalWindtunnel_ParameterSet_Visualizer;

class NumericalWindtunnel 
: public OpenFOAMAnalysis
{
  friend class NumericalWindtunnel_ParameterSet_Visualizer;

public:
  static void modifyDefaults(insight::ParameterSet& p);
#include "numericalwindtunnel__NumericalWindtunnel__Parameters.h"
/*
PARAMETERSET>>> NumericalWindtunnel Parameters
inherits OpenFOAMAnalysis::Parameters
addTo_makeDefault { modifyDefaults(p); }

geometry = set {

 LupstreamByL   = double 4 "[-] upstream domain extent, divided by object diagonal" *hidden
 LdownstreamByL = double 10 "[-] downstream domain extent, divided by object diagonal" *hidden
 LasideByL      = double 3 "[-] lateral domain extent, divided by object diagonal" *hidden
 LupByL         = double 3 "[-] height of the domain (above floor), divided by object diagonal" *hidden#

 transformation = set {
   forwarddir     = vector (1 0 0) "direction from rear to forward end in CAD geometry CS"
   upwarddir      = vector (0 0 1) "vertical direction in CAD geometry CS"
   localOrigin    = vector (0 0 0)
"Origin in CAD geometry CS.
 This determines the pivot point for rotations.
 Also, this point will be coincident with the floor, if verticalPlacement 'onFloor' is chosen
 or at the defined height above the floor, if 'atHeight' is selected."
 }

 verticalPlacement = selectablesubset {{
  onFloor set {}
  onCeiling set {}
  centered set {}
  atHeight set {
    height = selectablesubset {{
     relativeToDomain set {
       hByHdomain = double 0.5 "[-]"
     }
     absolute set {
       h = double 0. "[m] height above floor"
     }
    }} relativeToDomain ""
  }
 }} onFloor ""

 attitude = set {
  trim = double 0. "[deg] Trim angle which should applied to the positioned geometry. Around origin of imported geometry. Sequence of application is Roll > Trim > Yaw."
  roll = double 0. "[deg] Roll angle which should applied to the positioned geometry. Around origin of imported geometry. Sequence of application is Roll > Trim > Yaw."
  yaw = double 0. "[deg] Yaw angle which should applied to the positioned geometry. Around origin of imported geometry. Sequence of application is Roll > Trim > Yaw."
 } ""
 
 objects     = labeledarray "object%d" [
    cadgeometry "" "Path to object geometry. May be STL, STEP or IGES." *necessary
 ] *1
"Objects in the wind tunnel. All objects are transformed equally,
 thus they are expected to be initially already in the same coordinate system.
 For each object, the individual forces are reported seperately."
 
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

 v              = double 1.0 "[m/s] incident velocity (or bulk velocity in case of cyclic BC)" *necessary

 inflow = selectablesubset {{

  homogeneousInflow set {}

  cyclicBC set {}

 }} homogeneousInflow ""


 boundaryConditions = set {

   topWall = selection (fixedTopWall movingTopWall topSymmetryPlane) fixedTopWall
"boundary type of the upper end of the domain (ceiling)"

   bottomWall = selection (fixedBottomWall movingBottomWall bottomSymmetryPlane) movingBottomWall
"boundary type of the lower end of the domain (floor)"

   sides = selection (fixedSideWalls sideSymmetryPlane sideOutlet) fixedSideWalls
"boundary type of the lateral ends of the domain"

 }

} "Definition of the operation point under consideration"
      




fluid = set {

 rho            = double 1.0 "[kg/m^3] Density of the fluid"
 nu             = double 1.5e-5 "[m^2/s] Viscosity of the fluid"
 turbulenceModel = dynamicclassparameters "insight::turbulenceModel" default "kOmegaSST" "Turbulence model"
 
} "Parameters of the fluid"





eval = set
{
    referenceAreaProjectionDirection = selection (frontal lateral vertical) frontal
"A projected area of the geometry is used to calculate force coefficients. This selects the direction of the projection."

    allForces = set {
        includeTopWall = bool false "include upper wall (if not a symmetry plane) into to total force figure"
        includeBottomWall = bool false "include lower wall (if not a symmetry plane) into to total force figure"
    }

    evaluateMeanResistance = bool false "if set, the mean wall shear stress will be evaluated"
} "Parameters for evaluation after solver run"



<<<PARAMETERSET
*/

  struct supplementedInputData
      : public supplementedInputDataDerived<Parameters>
  {
  public:
    supplementedInputData(
          ParameterSetInput ip,
          const boost::filesystem::path& workDir,
          ActionProgress& progress );

    std::map<std::string,cad::FeaturePtr> geometry_;
    gp_Trsf cad_to_cfd_;
    double Lupstream_;
    double Ldownstream_;
    double Hdom_, Lup_, Ldown_;
    double Laside_;
    double Lref_, l_, w_, hup_, dlo_;

    arma::mat PiM_;

    const std::string FOname_allObjects;

    std::shared_ptr<bmd::blockMeshBlocking> blocking;
  };

  addParameterMembers_SupplementedInputData(NumericalWindtunnel::Parameters);

public:
  declareType("Numerical Wind Tunnel");
  
  NumericalWindtunnel(
      const std::shared_ptr<supplementedInputDataBase>& sp );

  
  void calcDerivedInputData(ProgressDisplayer& parentActionProgress) override;
  
  void createCase(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;
  void createMesh(insight::OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;

  ResultSetPtr evaluateResults(OpenFOAMCase& cm, ProgressDisplayer& parentActionProgress) override;

  static std::string category() { return "Generic Analyses"; }
  static AnalysisDescription description() { return {"Numerical Wind Tunnel", ""}; }
};




}

#endif
