#include "projectedface.h"

namespace insight {
namespace bmd {


ProjectedFace::ProjectedFace(const PointList& pts, const std::string& geometryLabel)
    : face_(pts),
      geometryLabel_(geometryLabel)
{
}

const PointList& ProjectedFace::face() const
{
    return face_;
}

const std::string& ProjectedFace::geometryLabel() const
{
    return geometryLabel_;
}


} // namespace bmd
} // namespace insight
