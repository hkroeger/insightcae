#include "block2d.h"

namespace insight {
namespace bmd {


Block2D::Block2D
(
  transform2D& t2d,
  PointList corners,
  int resx, int resy,
  GradingList grading,
  std::string zone,
  bool inv
)
: Block
  (
    (boost::assign::list_of
    (t2d.rvs(corners[0])),
    (t2d.rvs(corners[1])),
    (t2d.rvs(corners[2])),
    (t2d.rvs(corners[3])),
    (t2d.fwd(corners[0])),
    (t2d.fwd(corners[1])),
    (t2d.fwd(corners[2])),
    (t2d.fwd(corners[3]))),
    resx, resy, 1,
    (boost::assign::list_of
    (grading[0]), (grading[1]), (1)),
    zone,
    inv
  ),
  t2d_(t2d)

{
    t2d_.fwdPatch().addFace
    (
      t2d_.fwd(corners[3]),
      t2d_.fwd(corners[2]),
      t2d_.fwd(corners[1]),
      t2d_.fwd(corners[0])
    );

    t2d_.rvsPatch().addFace
    (
      t2d_.rvs(corners[0]),
      t2d_.rvs(corners[1]),
      t2d_.rvs(corners[2]),
      t2d_.rvs(corners[3])
    );
}

} // namespace bmd
} // namespace insight
