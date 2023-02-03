#ifndef INSIGHT_BMD_ARCEDGE_H
#define INSIGHT_BMD_ARCEDGE_H

#include "openfoam/blockmesh/edge.h"

namespace insight {
namespace bmd {

class ArcEdge
: public Edge
{
protected:
  Point midpoint_;

public:
  ArcEdge(const Point& c0, const Point& c1, const Point& midpoint);

  std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const override;

  Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const override;
  Edge* clone() const override;
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_ARCEDGE_H
