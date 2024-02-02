#include "splineedge.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

namespace insight {
namespace bmd {



SplineEdge::SplineEdge(
        const PointList& points,
        const std::string& splinekeyword,
        const std::string& geometryLabel)
: Edge(points.front(), points.back()),
  intermediatepoints_(points.begin()+1, points.end()-1),
  splinekeyword_(splinekeyword),
  geometryLabel_(geometryLabel)
{}


bmd::DiscreteCurve SplineEdge::allPoints() const
{
  bmd::DiscreteCurve ap;
  ap.push_back(c0_);
  copy( intermediatepoints_.begin(),
        intermediatepoints_.end(),
        std::back_inserter(ap) );
  ap.push_back(c1_);
  return ap;
}


std::vector<OFDictData::data> SplineEdge::bmdEntry(const PointMap& allPoints, int OFversion) const
{
  std::vector<OFDictData::data> l;

  if (geometryLabel_.empty())
  {
      if (OFversion<=160)
        l.push_back(splinekeyword_);
      else
        l.push_back("spline");
  }
  else
  {
      l.push_back("projectSpline");
  }

  l.push_back( OFDictData::data(allPoints.find(c0_)->second) );
  l.push_back( OFDictData::data(allPoints.find(c1_)->second) );

  OFDictData::list pl;
  for ( const Point& pt: intermediatepoints_ )
  {
    OFDictData::list ppl;
    ppl += OFDictData::data(pt[0]), OFDictData::data(pt[1]), OFDictData::data(pt[2]);
    pl.push_back(ppl);
  }
  l.push_back(pl);

  if (!geometryLabel_.empty())
  {
      l.push_back("("+geometryLabel_+")");
  }

  return l;
};


Edge* SplineEdge::transformed(const arma::mat& tm, const arma::mat trans) const
{
  PointList pl;
  pl+=tm*c0_+trans;
  for (const Point& p: intermediatepoints_)
  {
    pl+=tm*p+trans;
  }
  pl+=tm*c1_+trans;
  return new SplineEdge(pl, splinekeyword_);
}

Edge* SplineEdge::clone() const
{
  PointList pts;
  pts.push_back(c0_);
  copy(intermediatepoints_.begin(), intermediatepoints_.end(), std::back_inserter(pts));
  pts.push_back(c1_);
  return new SplineEdge(pts, splinekeyword_);
}



} // namespace bmd
} // namespace insight
