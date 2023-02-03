#ifndef INSIGHT_BMD_SPLINEEDGE_H
#define INSIGHT_BMD_SPLINEEDGE_H

#include "openfoam/blockmesh/edge.h"
#include "openfoam/blockmesh/discretecurve.h"

namespace insight {
namespace bmd {

class SplineEdge
: public Edge
{
protected:
  PointList intermediatepoints_;
  std::string splinekeyword_;
  std::string geometryLabel_;

public:
  SplineEdge(const PointList& points,
             const std::string& splinekeyword="simpleSpline",
             const std::string& geometryLabel="");

  virtual std::vector<OFDictData::data> bmdEntry(const PointMap& allPoints, int OFversion) const;

  virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const;
  virtual Edge* clone() const;

  bmd::DiscreteCurve allPoints() const;
};

} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_SPLINEEDGE_H
