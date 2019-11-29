#ifndef INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H
#define INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

namespace insight {

class rigidBodyMotionDynamicMesh
: public dynamicMesh
{
public:
#include "rigidbodymotiondynamicmesh__rigidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> rigidBodyMotionDynamicMesh Parameters

rho = selectablesubset {{
 field set {
  fieldname = string "rho" "Density field name"
 }
 constant set {
  rhoInf = double 1025.0 "Constant density value"
 }
}} constant "Density source"


bodies = array [
 set {
   name = string "movingbody" "Name of the body"
   centreOfMass = vector (0 0 0) "Location of CoG in global CS"
   mass = double 1.0 "Mass of body"
   Ixx = double 1.0 "Inertia Ixx"
   Iyy = double 1.0 "Inertia Iyy"
   Izz = double 1.0 "Inertia Izz"

   patches = array [
    string "bodysurface" "Names of patches comprising the surface of the body"
   ] *1 "body surface patches"

   innerDistance = double 1.0 "radius around body within which a solid body motion is performed."
   outerDistance = double 2.0 "radius around body outside which the grid remains fixed."

   translationConstraint = array [ selection (
    Px Py Pz Pxyz ) Pxyz "Kind of translation constraint"
   ] *1 "translation constraints"

   rotationConstraint = array [ selection (
    Rx Ry Rz Rxyz ) Rxyz "Kind of rotation constraint"
   ] *1 "rotation constraints"

 } ] *1 "moving bodies"


implementation = selectablesubset {{
 vanilla set { }
 extended set {
   rampDuration = double 1.0 "Duration of the initial force ramp"
 }
}} vanilla "Type of implementation to use."

<<<PARAMETERSET
*/

protected:
    ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

public:
  declareType ( "rigidBodyMotionDynamicMesh" );

  rigidBodyMotionDynamicMesh( OpenFOAMCase& c, const ParameterSet&ps = Parameters::makeDefault() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Dynamic Mesh"; }
};


} // namespace insight

#endif // INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H
