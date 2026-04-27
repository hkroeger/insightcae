#ifndef INSIGHT_DYNAMICOVERSETFVMESH_H
#define INSIGHT_DYNAMICOVERSETFVMESH_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"
#include "openfoam/caseelements/dynamicmesh/sixdofrigidbodymotionsolver.h"

#include "dynamicoversetfvmesh__dynamicOversetFvMesh__Parameters_headers.h"


namespace insight {

class dynamicOversetFvMesh
    : public rigidBodyMotionBasedDynamicMesh
{
public:
#include "dynamicoversetfvmesh__dynamicOversetFvMesh__Parameters.h"
/*
PARAMETERSET>>> dynamicOversetFvMesh Parameters
inherits rigidBodyMotionBasedDynamicMesh::Parameters

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "dynamicOversetFvMesh" );

  dynamicOversetFvMesh(
      OpenFOAMCase& c,
      ParameterSetInput ip = Parameters() );
  void addFields( OpenFOAMCase& c ) const override;
  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Dynamic Mesh"; }
};


} // namespace insight

#endif // INSIGHT_DYNAMICOVERSETFVMESH_H
