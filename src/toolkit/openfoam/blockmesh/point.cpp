#include "point.h"

namespace insight {
namespace bmd {




PointList P_4(const Point& p1, const Point& p2, const Point& p3, const Point& p4)
{
  return { p1, p2, p3, p4};
}




PointList P_4(const PointList& pts, int p1, int p2, int p3, int p4)
{
  return P_4(pts[p1], pts[p2], pts[p3], pts[p4]);
}




PointList P_8(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
          const Point& p5, const Point& p6, const Point& p7, const Point& p8)
{
  return {p1, p2, p3, p4, p5, p6, p7, p8};
}




PointList P_8_DZ(const Point& p1, const Point& p2, const Point& p3, const Point& p4,
                 const arma::mat& dz0, const arma::mat& dz1)
{
  return {
      p1+dz0, p2+dz0, p3+dz0, p4+dz0,
      p1+dz1, p2+dz1, p3+dz1, p4+dz1
      };
}




OFDictData::list bmdEntry(const PointList& pts, const PointMap& allPoints)
{
  OFDictData::list res;
  for (const auto& p: pts)
    {
      res.push_back( allPoints.find(p)->second );
    }
  return res;
}




} // namespace bmd
} // namespace insight
