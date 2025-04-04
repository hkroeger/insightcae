#include "dynamicmesh.h"

namespace insight {

dynamicMesh::dynamicMesh(OpenFOAMCase& c, ParameterSetInput ip)
: OpenFOAMCaseElement(c, /*"dynamicMesh", */ip.forward<Parameters>())
{
}

bool dynamicMesh::isUnique() const
{
  return true;
}

} // namespace insight
