#ifndef INSIGHT_SIXDOFRIGIDBODYMOTIONSOLVER_H
#define INSIGHT_SIXDOFRIGIDBODYMOTIONSOLVER_H

#include "openfoam/openfoamdict.h"

#include "sixdofrigidbodymotionsolver__SixDOFRigidBodyMotionSolver__Parameters_headers.h"

namespace insight {


class SixDOFRigidBodyMotionSolver
{
public:
#include "sixdofrigidbodymotionsolver__SixDOFRigidBodyMotionSolver__Parameters.h"

/*
PARAMETERSET>>> SixDOFRigidBodyMotionSolver Parameters

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

   joints = array [ selection (
    Px Py Pz Pxyz Rx Ry Rz Rxyz
   ) Pz "Kind of joints"
   ] *1 "joints"

 } ] *1 "moving bodies"


implementation = selectablesubset {{

 vanilla set { }

 extended set {

   rampDuration = double 1.0 "Duration of the initial force ramp"

   directForces = array [ set {
    bodyName = string "" "name of the body"
    PoA = vector (0 0 0) "point of attack"
    forceSource = string "" "force source definition"
    coordinateSystemName = string "global" ""
    localDirection = vector ( 0 0 0 ) "local direction"
    verticalDirection = vector ( 0 0 0 ) "vertical direction"
   } ] *0 "additional forces"

 }

 maneuvering set {

   rampDuration = double 1.0 "Duration of the initial force ramp"

   directForces = array [ set {
    bodyName = string "" "name of the body"
    PoA = vector (0 0 0) "point of attack"
    forceSource = string "" "force source definition"
    coordinateSystemName = string "global" ""
    localDirection = vector ( 0 0 0 ) "local direction"
    verticalDirection = vector ( 0 0 0 ) "vertical direction"
   } ] *0 "additional forces"

   rudders = array [ set {
    cellZone = string "rudder" "" *necessary
    origin = vector (0 0 0) ""
    axis = vector (0 0 -1) ""
    maxRudderSpeed = double 0.043633231 "[rad/s] maximum rudder speed"  # 2.5 deg/s = (35deg - (-35deg)) / (28s)
   } ] *0 "rudders"

   relativeMovingBodies = array[ set {
   } ] *0 "bodies that move relative to the main body"
 }
}} vanilla "Type of implementation to use."


restraints = array [ selectablesubset {{
 prescribedVelocity set {
  label = string "fixSpeed" "unique label"
  body = string "" "name of body which speed is to be fixed" *necessary
  velocity = vector (1 0 0) "target velocity" *necessary
 }
}} prescribedVelocity "" ] *0 "restraint to apply"

createGetter
<<<PARAMETERSET
*/

protected:
    // ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

    template<class P>
    void insertExtendedMotionParameters(OFDictData::dict& rbmc, const Parameters& p, const P& emp) const
    {
        rbmc["rampDuration"]=emp.rampDuration;

        auto& bodyList = rbmc.subDict("bodies");

        for (auto& b: bodyList)
        {
            bodyList.subDict(b.first)["directThrustForces"]=
                    OFDictData::list();
        }

        for (const auto& df: emp.directForces)
        {
            auto ib = std::find_if(
                        p.bodies.begin(),
                        p.bodies.end(),
                        [&](const Parameters::bodies_default_type& b)
                        {
                            return b.name==df.bodyName;
                        } );
            int iforce = &df-&emp.directForces.front();
            insight::assertion(
                        ib!=p.bodies.end(),
                        str(boost::format("direct force #%d references non-existing body %s!")
                            % iforce % df.bodyName) );

            OFDictData::dict dfp;
            dfp["PoA"]=OFDictData::vector3(df.PoA);
            dfp["forceSource"]=df.forceSource;
            dfp["coordinateSystemName"]=df.coordinateSystemName;
            bodyList.subDict(df.bodyName).getList("directThrustForces").push_back(dfp);
        }
    }

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
