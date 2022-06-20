#include "multipliedforcesource.h"

namespace Foam {

multipliedForceSource::multipliedForceSource(const word& lbl, scalar m, forceSource* f, bool autoRegister)
    : forceSource(lbl, autoRegister),
      multiplier_(m),
      f_(f)
{}

vector multipliedForceSource::force() const
{
    return multiplier_ * f_->force();
}

} // namespace Foam
