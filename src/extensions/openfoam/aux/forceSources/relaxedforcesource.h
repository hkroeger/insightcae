#ifndef FOAM_RELAXEDFORCESOURCE_H
#define FOAM_RELAXEDFORCESOURCE_H

#include "forcesources.h"
#include "fvCFD.H"

namespace Foam {


class relaxedForceSource
        : public forceSource
{
protected:
    scalar relaxationFactor_;
    mutable vector lastForce_;
    forceSource *f_;
    const Time& time_;
    mutable label lastTimeIndex_;

public:
    relaxedForceSource(
            const word& lbl,
            const Time& time,
            scalar m, const vector& f0,
            forceSource* f, bool autoRegister=true);

    vector force() const override;
};


} // namespace Foam

#endif // FOAM_RELAXEDFORCESOURCE_H
