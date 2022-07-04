#ifndef FOAM_EXTRBM_DIRECTTHRUSTFORCE_H
#define FOAM_EXTRBM_DIRECTTHRUSTFORCE_H

#include "point.H"
#include "vector.H"

#include "additionalforceandmoment.h"
#include "forcesourcecombination.h"

#include <functional>


namespace Foam {
namespace extRBM {



class directForce;



class directForceFactory
{
    const Time& time_;
public:
    directForceFactory(const Time& time);
    autoPtr<directForce> operator()(Istream& is) const;
};




class directForce
        : public additionalForceAndMoment
{
    point PoA_;

    /**
     * @brief localDirection_
     * if magnitude is >0, force source will be projected on this vector
     * to yield actual force. The direction vector will be rotated with the body
     * before the projection.
     */
    vector localDirection_;
    vector verticalConstraint_;
    forceSourceCombination fs_;

    word coordinateSystemName_;

public:
    TypeName("directForce");

//    static autoPtr<directForce> New(Istream& is);

    directForce(
            const point& PoA,
            const vector& localDirection,
            const vector& verticalDirection,
            const forceSourceCombination& fs,
            const word& coordinateSystemName = "global" );

//    std::pair<point,vector> PoAAndForce(std::function<point(point)> transformedPoint) const;
    std::pair<vector,vector> forceAndMoment() const override;

    autoPtr<directForce> clone() const;
};

} // namespace extRBM
} // namespace Foam

#endif // FOAM_EXTRBM_DIRECTTHRUSTFORCE_H
