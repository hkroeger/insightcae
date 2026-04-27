#ifndef INSIGHT_DYNAMICMESH_H
#define INSIGHT_DYNAMICMESH_H

#include "openfoam/caseelements/openfoamcaseelement.h"
#include "openfoam/caseelements/dynamicmesh/sixdofrigidbodymotionsolver.h"

namespace insight {

class dynamicMesh
: public OpenFOAMCaseElement
{
public:
  dynamicMesh(OpenFOAMCase& c, ParameterSetInput ip = Parameters() );
  static std::string category() { return "Dynamic Mesh"; }
  virtual bool isUnique() const;
};



class rigidBodyMotionBasedDynamicMesh
: public dynamicMesh
{
public:
#include "dynamicmesh__rigidBodyMotionBasedDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> rigidBodyMotionBasedDynamicMesh Parameters
inherits dynamicMesh::Parameters

rigidBodyMotion = includedset "SixDOFRigidBodyMotionSolver::Parameters" "Parameters of the rigid body motion solver"

createGetter
<<<PARAMETERSET
*/

public:
    declareType ( "rigidBodyMotionBasedDynamicMesh" );

    rigidBodyMotionBasedDynamicMesh(
        OpenFOAMCase& c,
        ParameterSetInput ip = Parameters() );
};

} // namespace insight

#endif // INSIGHT_DYNAMICMESH_H
