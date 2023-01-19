#ifndef INSIGHT_LSDYNAINPUTCARDS_BOUNDARYCONDITIONS_H
#define INSIGHT_LSDYNAINPUTCARDS_BOUNDARYCONDITIONS_H


#include "lsdyna/lsdynainputcard.h"
#include "base/units.h"

namespace insight {
namespace LSDynaInputCards {


class Part;
class Curve;


class BoundaryPrescribedMotionRigid
        : public LSDynaInputCard
{
public:
    enum DOF {

        X = 1,
            //X translation.
        Y = 2,
            //Y translation.
        Z = 3,
            //Z translation.
        Xvec= 4,
            // X translation in the direction defined by VID.
        XvecNormal = -4,
            // X translation in the direction defined by VID and fixed motion in the normal plane.
        RX = 5,
            //XX rotation.
        RY = 6,
            //YY rotation.
        RZ = 7,
            //ZZ rotation.
        RXvec = 8,
            //XX rotation in the direction defined by VID.
        RXvecNormal = -8
            //XX rotation in the direction defined by VID and fixed rotation in the normal plane.
    };

    enum MotionType {
        Velocity = 0,
            //Imposed velocity.
        Acceleration = 1,
            //Imposed acceleration.
        Displacement = 2
            //Imposed displacement.
    };


private:
    const Part *part_;
    DOF dof_;
    MotionType motionType_;
    const Curve *curve_;
    double curveScaleY_;
    si::Time tStop_, tStart_;

public:
    BoundaryPrescribedMotionRigid(
            const Part *part,
            DOF dof,
            MotionType motionType,
            const Curve *curve,
            double curveScaleY = 1.,
            si::Time tStop = 1e30*si::seconds, si::Time tStart = 0
            );

    void write(std::ostream& os) const override;
};



} // namespace LSDynaInputCards
} // namespace insight

#endif // INSIGHT_LSDYNAINPUTCARDS_BOUNDARYCONDITIONS_H
