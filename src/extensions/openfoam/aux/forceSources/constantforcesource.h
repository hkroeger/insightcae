#ifndef FOAM_CONSTANTFORCESOURCE_H
#define FOAM_CONSTANTFORCESOURCE_H

#include "forcesources.h"

namespace Foam {

class constantForceSource
        : public forceSource
{
protected:
    vector F_;

public:
    constantForceSource(const word& lbl, const vector& F, bool autoRegister=true);

    vector force() const override;
};

} // namespace Foam

#endif // FOAM_CONSTANTFORCESOURCE_H
