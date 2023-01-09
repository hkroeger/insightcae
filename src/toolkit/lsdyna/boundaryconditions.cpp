#include "boundaryconditions.h"

#include "lsdyna/mesh.h"
#include "lsdynainputdeck.h"

namespace insight {
namespace LSDynaInputCards {




BoundaryPrescribedMotionRigid::BoundaryPrescribedMotionRigid(
        const Part *part,
        DOF dof,
        MotionType motionType,
        const Curve *curve,
        double curveScaleY,
        si::Time tStop, si::Time tStart
        )
    : part_(part),
      dof_(dof),
      motionType_(motionType),
      curve_(curve),
      curveScaleY_(curveScaleY),
      tStop_(tStop), tStart_(tStart)
{}

void BoundaryPrescribedMotionRigid::write(std::ostream& os) const
{
    os << "*BOUNDARY_PRESCRIBED_MOTION_RIGID\n"
       << part_->id() << ", "
       << int(dof_) <<", "
       << int(motionType_) << ", "
       << curveScaleY_ << ", "
       << 0 << ", "
       << toValue(tStop_, inputDeck().timeUnit()) << ", "
       << toValue(tStart_, inputDeck().timeUnit()) << "\n";
}




} // namespace LSDynaInputCards
} // namespace insight
