#ifndef INSIGHT_SIXDOFRIGIDBODYMOTIONSOLVER_H
#define INSIGHT_SIXDOFRIGIDBODYMOTIONSOLVER_H

#include "openfoam/openfoamdict.h"
#include "openfoam/caseelements/dynamicmesh/solidbodymotionfunction.h"
#include "sixdofrigidbodymotionsolver__SixDOFRigidBodyMotionSolver__Parameters_headers.h"

namespace insight {



class relativeMovingBody
{
public:
#include "sixdofrigidbodymotionsolver__relativeMovingBody__Parameters.h"
/*
PARAMETERSET>>> relativeMovingBody Parameters
inherits insight::ParametersBase
inherits insight::solidBodyMotionFunction::Parameters

bodyName = string "" "name of the body, relative to which the object moves"

region = selectablesubset {{
 cellZone set {
  zoneName = string "" "name of zone with cells to move"
 }
 cellSet set {
  setName = string "" "name of set with cells to move"
 }
}} cellZone ""

createGetter
<<<PARAMETERSET
*/
    ParameterSetInput p_;

public:
    relativeMovingBody(ParameterSetInput ip);

    void addIntoDictionary(OFDictData::dict& dictionary) const;
};




class ExtendedMotion
{
public:
#include "sixdofrigidbodymotionsolver__ExtendedMotion__Parameters.h"
/*
PARAMETERSET>>> ExtendedMotion Parameters
skipDefaultBase

rampDuration = double 1.0 "Duration of the initial force ramp"
referenceSystemSpeed = vector (0 0 0) "direction and velocity of inertial system"

directForces = array [ set {
    bodyName = string "" "name of the body"
    PoA = vector (0 0 0) "point of attack"
    forceSource = string "" "force source definition"
    coordinateSystemName = string "global" ""
    localDirection = vector ( 0 0 0 ) "local direction"
    verticalDirection = vector ( 0 0 0 ) "vertical direction"
} ] *0 "additional forces"

relativeMovingBodies = array [
    includedset "insight::relativeMovingBody::Parameters"
] *0 "bodies that move relative to the main body"

<<<PARAMETERSET
*/


    // need forward declaration of SixDOFRigidBodyMotionSolver::Parameters => not possible
    // static void insertExtendedMotionParameters(
    //     OFDictData::dict& rbmc,
    //     const Parameters& p, const P& emp) const
};




class ManeuveringMotion
{
public:
#include "sixdofrigidbodymotionsolver__ManeuveringMotion__Parameters.h"
/*
PARAMETERSET>>> ManeuveringMotion Parameters
inherits* insight::ExtendedMotion::Parameters

rudders = array [ set {
    cellZone = string "rudder" "" *necessary
    origin = vector (0 0 0) ""
    axis = vector (0 0 -1) ""
    maxRudderSpeed = double 0.043633231 "[rad/s] maximum rudder speed"  # 2.5 deg/s = (35deg - (-35deg)) / (28s)
} ] *0 "rudders"

<<<PARAMETERSET
*/
};




class SixDOFRigidBodyMotionSolver
{
public:
#include "sixdofrigidbodymotionsolver__SixDOFRigidBodyMotionSolver__Parameters.h"
/*
PARAMETERSET>>> SixDOFRigidBodyMotionSolver Parameters

accelerationRelaxation = double 0.7 "relaxation of acceleration in motion solver"
accelerationDamping = double 1.0 "damping of acceleration and velocity in motion solver"

rho = selectablesubset {{
 field set {
  fieldname = string "rho" "Density field name"
 }
 constant set {
  rhoInf = double 1025.0 "Constant density value"
 }
}} field "Density source"


bodies = labeledarray "body%d" [
 set {
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

   joints = array [ selection (
    Px Py Pz Pxyz Rx Ry Rz Rxyz
   ) Pz "Kind of joints"
   ] *1 "joints"

   transform = matrix [ [1,0,0], [0,1,0], [0,0,1] ] "Rotation matrix"

 } ] *0 "moving bodies"


implementation = selectablesubset {{

 vanilla set { }

 extended includedset "insight::ExtendedMotion::Parameters"

 maneuvering includedset "insight::ManeuveringMotion::Parameters"

}} vanilla "Type of implementation to use."


restraints = labeledarray "restraint%d" [ selectablesubset {{
 prescribedVelocity set {
  body = string "" "name of body which speed is to be fixed" *necessary
  velocity = vector (1 0 0) "target velocity" *necessary
 }

 linearDamper set {
  body = string "" "Body to damp"
  coeff = double 0.0 "damping coefficient"
 }

 sphericalAngularDamper set {
  body = string "" "Body to damp"
  coeff = double 0.0 "damping coefficient"
 }

}} prescribedVelocity "" ] *0 "restraint to apply"

createGetter
<<<PARAMETERSET
*/

protected:
    // ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

    void insertExtendedMotionParameters(
        OFDictData::dict& rbmc,
        const Parameters& p,
        const ExtendedMotion::Parameters& emp) const;

public:
  declareType ( "SixDOFRigidBodyMotionSolver" );

public:
    SixDOFRigidBodyMotionSolver(
        ParameterSetInput ip = Parameters() );

    virtual void addIntoDict(OFDictData::dict& dict) const;

    virtual std::string motionSolverName() const;
};

} // namespace insight

#endif // INSIGHT_SIXDOFRIGIDBODYMOTIONSOLVER_H
