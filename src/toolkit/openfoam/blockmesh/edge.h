#ifndef INSIGHT_BMD_EDGE_H
#define INSIGHT_BMD_EDGE_H

#include "openfoam/blockmesh/point.h"

namespace insight {
namespace bmd {

class blockMesh;

class Edge
{
protected:
  Point c0_, c1_;

public:
  Edge(const Point& c0, const Point& c1);
  virtual ~Edge();

  bool connectsPoints(const Point& c0, const Point& c1) const;
  inline const Point& c0() const { return c0_; }
  inline const Point& c1() const { return c1_; }

  virtual std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const =0;

  virtual void registerPoints(blockMesh& bmd) const;

  virtual Edge* transformed(const arma::mat& tm, const arma::mat trans=vec3(0,0,0)) const =0;
  virtual Edge* clone() const =0;

};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_EDGE_H
