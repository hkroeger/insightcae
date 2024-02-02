#ifndef INSIGHT_BMD_BLOCK2D_H
#define INSIGHT_BMD_BLOCK2D_H


#include "openfoam/blockmesh/block.h"
#include "openfoam/blockmesh/transform2d.h"

namespace insight {
namespace bmd {


class Block2D
: public Block
{
protected:
  transform2D& t2d_;

public:
    Block2D
    (
      transform2D& t2d,
      PointList corners,
      int resx, int resy,
      GradingList grading = GradingList(3, 1),
      std::string zone="",
      bool inv=false
    );
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCK2D_H
