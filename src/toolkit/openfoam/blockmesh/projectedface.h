#ifndef INSIGHT_BMD_PROJECTEDFACE_H
#define INSIGHT_BMD_PROJECTEDFACE_H

#include "openfoam/blockmesh/point.h"

namespace insight {
namespace bmd {

class ProjectedFace
{
    PointList face_;
    std::string geometryLabel_;

public:
    ProjectedFace(const PointList& pts, const std::string& geometryLabel);

    const PointList& face() const;
    const std::string& geometryLabel() const;
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PROJECTEDFACE_H
