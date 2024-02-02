#include "relaxedforcesource.h"

namespace Foam {

relaxedForceSource::relaxedForceSource(
        const word& lbl,
        const Time& time,
        scalar m, const vector& f0,
        forceSource* f, bool autoRegister )
    : forceSource(lbl, autoRegister),
      relaxationFactor_(m),
      lastForce_(f0),
      f_(f),
      time_(time),
      lastTimeIndex_(time_.timeIndex())
{}

vector relaxedForceSource::force() const
{
    vector fCur =
            relaxationFactor_ * f_->force()
            +
            (1. - relaxationFactor_) * lastForce_
            ;
    if (time_.timeIndex()!=lastTimeIndex_)
    {
        lastTimeIndex_=time_.timeIndex();
        lastForce_=fCur;
    }
    return fCur;
}

} // namespace Foam
