#include "ellipseedge.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

namespace insight {
namespace bmd {


EllipseEdge::EllipseEdge
(
  const Point& c0, const Point& c1,
  const Point& center,
  double ex
)
: Edge(c0, c1),
  center_(center),
  ex_(ex)
{
}


std::vector<OFDictData::data>
EllipseEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;
  l.push_back( OFDictData::data("arc") );
  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );
  OFDictData::list pl;
  pl += OFDictData::data(center_[0]), OFDictData::data(center_[1]), OFDictData::data(center_[2]);
  l.push_back(pl);
  l.push_back( OFDictData::data(ex_) );
  return l;
}

Edge* EllipseEdge::transformed(const arma::mat& tm, const arma::mat trans) const
{
  throw insight::Exception("Not implemented!");
}

Edge* EllipseEdge::clone() const
{
  return new EllipseEdge(c0_, c1_, center_, ex_);
}


} // namespace bmd
} // namespace insight
