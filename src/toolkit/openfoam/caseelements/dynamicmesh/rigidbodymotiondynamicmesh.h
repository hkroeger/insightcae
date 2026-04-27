#ifndef INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H
#define INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"

#include "rigidbodymotiondynamicmesh__rigidBodyMotionDynamicMesh__Parameters_headers.h"

namespace insight {

class rigidBodyMotionDynamicMesh
: public rigidBodyMotionBasedDynamicMesh
{
public:
#include "rigidbodymotiondynamicmesh__rigidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> rigidBodyMotionDynamicMesh Parameters
inherits rigidBodyMotionBasedDynamicMesh::Parameters

moveMeshOuterCorrectors = bool false "Whether the mesh motion is updated in every outer iteration within a time step. If set to false, mesh motion is updated only at the end of the time step."

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "rigidBodyMotionDynamicMesh" );

  rigidBodyMotionDynamicMesh(
      OpenFOAMCase& c,
      ParameterSetInput ip = Parameters() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Dynamic Mesh"; }
};


} // namespace insight

#endif // INSIGHT_RIGIDBODYMOTIONDYNAMICMESH_H
