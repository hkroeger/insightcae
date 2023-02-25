#include "singleedgefeature.h"

namespace insight {
namespace cad {


int ConstrainedSketchGeometry::nDoF() const
{
    return 0;
}

double ConstrainedSketchGeometry::getDoFValue(unsigned int iDoF) const
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
    return NAN;
}

void ConstrainedSketchGeometry::setDoFValue(unsigned int iDoF, double value)
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
}


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
