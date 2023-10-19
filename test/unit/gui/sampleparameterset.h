#ifndef SAMPLEPARAMETERSET_H
#define SAMPLEPARAMETERSET_H

#include "sampleparameterset__TestPS__Parameters_headers.h"

namespace insight
{
namespace TestPS
{

#include "sampleparameterset__TestPS__Parameters.h"
/*
PARAMETERSET>>> TestPS Parameters

geometry = set {

 LupstreamByL   = double 4 "[-] upstream domain extent, divided by object diagonal" *hidden
 LdownstreamByL = double 10 "[-] downstream domain extent, divided by object diagonal" *hidden
 LasideByL      = double 3 "[-] lateral domain extent, divided by object diagonal" *hidden
 LupByL         = double 3 "[-] height of the domain (above floor), divided by object diagonal" *hidden
 forwarddir     = vector (1 0 0) "direction from rear to forward end in CAD geometry CS"
 upwarddir      = vector (0 0 1) "vertical direction in CAD geometry CS"

 objectfile     = path "" "Path to object geometry. May be STL, STEP or IGES." *necessary

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

 refinementZones = set {  #11
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
 }

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

}

}
#endif // SAMPLEPARAMETERSET_H
