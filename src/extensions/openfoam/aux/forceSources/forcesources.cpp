#include "forcesources.h"

#include <exception>

namespace Foam {


forceSource::forceSource(const word& lbl, bool autoRegister)
    : globalObject<forceSources>(lbl, autoRegister)
{}

scalar forceSource::torque() const
{
    throw std::exception();
}



} // namespace Foam
