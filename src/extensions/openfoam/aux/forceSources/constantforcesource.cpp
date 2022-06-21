#include "constantforcesource.h"

namespace Foam {



constantForceSource::constantForceSource(
        const word& lbl,
        const vector& F,
        bool autoRegister )
    : forceSource(lbl, autoRegister),
      F_(F)
{
}

vector constantForceSource::force() const
{
    return F_;
}


} // namespace Foam
