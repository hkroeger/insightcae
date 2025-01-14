#ifndef INSIGHT_DYNAMICMESH_H
#define INSIGHT_DYNAMICMESH_H

#include "openfoam/caseelements/openfoamcaseelement.h"

namespace insight {

class dynamicMesh
: public OpenFOAMCaseElement
{
public:
  dynamicMesh(OpenFOAMCase& c, ParameterSetInput ip = ParameterSetInput() );
  static std::string category() { return "Dynamic Mesh"; }
  virtual bool isUnique() const;
};

} // namespace insight

#endif // INSIGHT_DYNAMICMESH_H
