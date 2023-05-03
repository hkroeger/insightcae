#include "singleedgefeature.h"

namespace insight {
namespace cad {



bool SingleEdgeFeature::isSingleEdge() const
{
    return true;
}


bool SingleEdgeFeature::isSingleOpenWire() const
{
  return true;
}

} // namespace cad
} // namespace insight
