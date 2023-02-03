#ifndef INSIGHT_BMD_ELLIPSEEDGE_H
#define INSIGHT_BMD_ELLIPSEEDGE_H

#include "openfoam/blockmesh/edge.h"

namespace insight {
namespace bmd {

class EllipseEdge
: public Edge
{
protected:
  Point center_;
  double ex_;

public:
    EllipseEdge
    (
      const Point& c0, const Point& c1,
      const Point& center,
      double ex
    );

    virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

    virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const;
    virtual Edge* clone() const;
};

} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_ELLIPSEEDGE_H
