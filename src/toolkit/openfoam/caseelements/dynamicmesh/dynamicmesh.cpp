#include "dynamicmesh.h"

namespace insight {

dynamicMesh::dynamicMesh(OpenFOAMCase& c, const ParameterSet& ps)
: OpenFOAMCaseElement(c, "dynamicMesh", ps)
{
}

bool dynamicMesh::isUnique() const
{
  return true;
}

} // namespace insight
