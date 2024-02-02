#include "circularedge.h"

namespace insight {
namespace bmd {



CircularEdge::CircularEdge
(
  const Point& c0, const Point& c1,
  Point startPoint,
  arma::mat axis
)
: ArcEdge(c0, c1, vec3(0,0,0))
{
    arma::mat bp = arma::mat(startPoint)
            + axis*dot(axis, ( arma::mat(c0_) - arma::mat(startPoint) ));
    arma::mat r1 = arma::mat(c0_) - bp;
    double r = sqrt(dot(r1, r1));
    arma::mat mp0 = 0.5*(( arma::mat(c0_) - bp) + ( arma::mat(c1_) - bp));
    mp0 /= sqrt(dot(mp0,mp0));
    midpoint_ = arma::mat(bp) + r*mp0;
}


} // namespace bmd
} // namespace insight
