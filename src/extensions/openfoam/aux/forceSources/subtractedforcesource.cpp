#include "subtractedforcesource.h"

namespace Foam {

subtractedForceSource::subtractedForceSource(const word &lbl, forceSource *a, forceSource *b, bool autoRegister)
    : forceSource(lbl, autoRegister),
      a_(a), b_(b)
{}

Foam::vector subtractedForceSource::force() const
{
    return a_->force() - b_->force();
}

} // namespace Foam
