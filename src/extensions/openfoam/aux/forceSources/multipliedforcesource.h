#ifndef FOAM_MULTIPLIEDFORCESOURCE_H
#define FOAM_MULTIPLIEDFORCESOURCE_H

#include "forcesources.h"


namespace Foam {

class multipliedForceSource
        : public forceSource
{
protected:
    scalar multiplier_;
    forceSource *f_;

public:
    multipliedForceSource(const word& lbl, scalar m, forceSource* f, bool autoRegister=true);

    vector force() const override;
};

} // namespace Foam

#endif // FOAM_MULTIPLIEDFORCESOURCE_H
