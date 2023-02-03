#include "patch2d.h"

namespace insight {
namespace bmd {



Patch2D::Patch2D(const transform2D& t2d, std::string typ)
: Patch(typ),
  t2d_(t2d)
{
}

void Patch2D::addFace(const Point& c0, const Point& c1)
{
    Patch::addFace
    (
      t2d_.fwd(c0),
      t2d_.fwd(c1),
      t2d_.rvs(c1),
      t2d_.rvs(c0)
    );
}



} // namespace bmd
} // namespace insight
