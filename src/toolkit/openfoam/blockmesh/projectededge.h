#ifndef INSIGHT_BMD_PROJECTEDEDGE_H
#define INSIGHT_BMD_PROJECTEDEDGE_H

#include "openfoam/blockmesh/edge.h"

namespace insight {
namespace bmd {

class ProjectedEdge
        : public Edge
{
    std::string geometryLabel_;
public:
  ProjectedEdge(const Point& c0, const Point& c1, const std::string& geometryLabel);

  std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const override;

  Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const override;
  Edge* clone() const override;
};

} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PROJECTEDEDGE_H
