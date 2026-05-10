#ifndef INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H
#define INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H

#include "openfoam/caseelements/dynamicmesh/dynamicmesh.h"
#include "openfoam/caseelements/dynamicmesh/solidbodymotionfunction.h"
#include "solidbodymotiondynamicmesh__solidBodyMotionDynamicMesh__Parameters_headers.h"

namespace insight {



class solidBodyMotionDynamicMesh
: public dynamicMesh,
  public solidBodyMotionFunction
{
public:
#include "solidbodymotiondynamicmesh__solidBodyMotionDynamicMesh__Parameters.h"
/*
PARAMETERSET>>> solidBodyMotionDynamicMesh Parameters
inherits dynamicMesh::Parameters
inherits insight::solidBodyMotionFunction::Parameters

zonename = string "none" "Name of the cell zone which moves.
Enter 'none', if the entire mesh shall be moved."

createGetter
<<<PARAMETERSET
*/

public:
  declareType ( "solidBodyMotionDynamicMesh" );

  solidBodyMotionDynamicMesh(
      OpenFOAMCase& c,
      ParameterSetInput ip = solidBodyMotionDynamicMesh::Parameters() );

  void addIntoDictionaries(OFdicts& dictionaries) const override;

  static std::string category() { return "Dynamic Mesh"; }
};


} // namespace insight

#endif // INSIGHT_SOLIDBODYMOTIONDYNAMICMESH_H
