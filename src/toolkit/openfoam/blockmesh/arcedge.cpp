#include "arcedge.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

namespace insight {
namespace bmd {



ArcEdge::ArcEdge
(
  const Point& c0,
  const Point& c1,
  const Point& midpoint
)
: Edge(c0, c1),
  midpoint_(midpoint)
{
}


std::vector<OFDictData::data>
ArcEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;
  l.push_back( OFDictData::data("arc") );
  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );
  OFDictData::list pl;
  pl += OFDictData::data(midpoint_[0]), OFDictData::data(midpoint_[1]), OFDictData::data(midpoint_[2]);
  l.push_back(pl);
  return l;
}


Edge* ArcEdge::transformed(const arma::mat& tm, const arma::mat trans) const
{
  return new ArcEdge(tm*c0_+trans, tm*c1_+trans, tm*midpoint_+trans);
}

Edge* ArcEdge::clone() const
{
  return new ArcEdge(c0_, c1_, midpoint_);
}



} // namespace bmd
} // namespace insight
