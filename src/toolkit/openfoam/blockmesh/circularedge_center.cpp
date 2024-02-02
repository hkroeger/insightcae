#include "circularedge_center.h"

namespace insight {
namespace bmd {

CircularEdge_Center::CircularEdge_Center
(
  const Point& c0, const Point& c1,
  const Point& center
)
: ArcEdge(c0, c1, vec3(0,0,0))
{
  double radius=arma::norm(c0-center, 2);
  double r2=arma::norm(c1-center, 2);
  if (fabs(radius-r2)>1e-6)
    throw insight::Exception("The two points on the circular edge do not have the same distance from the center!");

  arma::mat mp=0.5*(c0+c1);
  arma::mat rm=mp-center; rm/=arma::norm(rm, 2);
  midpoint_ = center + rm*radius;
}

} // namespace bmd
} // namespace insight
