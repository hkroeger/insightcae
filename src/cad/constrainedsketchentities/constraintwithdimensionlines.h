#ifndef CONSTRAINTWITHDIMENSIONLINES_H
#define CONSTRAINTWITHDIMENSIONLINES_H

#include "constrainedsketchentity.h"

namespace insight {
namespace cad {


class ConstraintWithDimensionLines
: public ConstrainedSketchEntity
{
public:
    using ConstrainedSketchEntity::ConstrainedSketchEntity;

    virtual void setArrowSize(double absoluteArrowSize) =0;
};

} // namespace cad
} // namespace insight

#endif // CONSTRAINTWITHDIMENSIONLINES_H
