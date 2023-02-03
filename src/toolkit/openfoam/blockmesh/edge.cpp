#include "edge.h"

#include "openfoam/blockmesh.h"

namespace insight {
namespace bmd {


Edge::Edge(const Point& c0, const Point& c1)
: c0_(c0), c1_(c1)
{
}

Edge::~Edge()
{}

bool Edge::connectsPoints(const Point& c0, const Point& c1) const
{
  return (
      ( ( arma::norm(c0 - c0_, 2) < SMALL ) && ( arma::norm(c1 - c1_, 2) < SMALL ) )
      ||
      ( ( arma::norm(c1 - c0_, 2) < SMALL ) && ( arma::norm(c0 - c1_, 2) < SMALL ) )
     );
}

void Edge::registerPoints(blockMesh& bmd) const
{
  bmd.addPoint(c0_);
  bmd.addPoint(c1_);
}

} // namespace bmd
} // namespace insight
