#ifndef INSIGHT_BMD_POINT_H
#define INSIGHT_BMD_POINT_H

#include "base/linearalgebra.h"
#include "openfoam/openfoamdict.h"

#include <vector>
#include <map>

namespace insight {
namespace bmd {




typedef arma::mat Point;




typedef std::vector<Point> PointList;
typedef std::map<Point, int> PointMap;




PointList P_4(const Point& p1, const Point& p2, const Point& p3, const Point& p4);
PointList P_4(const PointList& pts, int p1, int p2, int p3, int p4);
PointList P_8(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
          const Point& p5, const Point& p6, const Point& p7, const Point& p8);
PointList P_8_DZ(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
              const arma::mat& dz0, const arma::mat& dz1);



OFDictData::list bmdEntry(const PointList& pts, const PointMap& allPoints);



} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_POINT_H
