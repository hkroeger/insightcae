#ifndef INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H
#define INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"
#include "solidbodymotiondynamicmesh__solidBodyMotionDynamicMesh__Parameters_headers.h"

namespace insight {

class solidBodyMotionDynamicMesh
: public dynamicMesh
{
public:
#include "solidbodymotiondynamicmesh__solidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> solidBodyMotionDynamicMesh Parameters
inherits dynamicMesh::Parameters

zonename = string "none" "Name of the cell zone which moves.
Enter 'none', if the entire mesh shall be moved."

motion = selectablesubset
{{

 rotation
 set {
  origin = vector (0 0 0) "origin point"
  axis = vector (0 0 1) "rotation axis"
  rpm = double 1000 "rotation rate"
 }

 translation
 set {
  velocity = vector (1 0 0) "motion velocity"
 }

 oscillatingRotating
 set {
  origin = vector (0 0 0) "origin point"
  amplitude = vector (0 0 1) "[deg] amplitude"
  omega = double 1 "[rad/sec] rotation frequency"
 }

}} rotation "type of motion"

createGetter
<<<PARAMETERSET
*/

// protected:
//     ParameterSet ps_; // need to use dynamic variant; will contain enhancements to above definition

public:
  declareType ( "solidBodyMotionDynamicMesh" );

  solidBodyMotionDynamicMesh(
      OpenFOAMCase& c,
      ParameterSetInput ip = Parameters() );

  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Dynamic Mesh"; }
};


} // namespace insight

#endif // INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H
