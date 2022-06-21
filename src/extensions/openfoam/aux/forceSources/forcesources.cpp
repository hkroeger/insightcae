#include "forcesources.h"

namespace Foam {


forceSource::forceSource(const word& lbl, bool autoRegister)
    : globalObject<forceSources>(lbl, autoRegister)
{}



} // namespace Foam
