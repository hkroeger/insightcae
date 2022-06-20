#ifndef FOAM_SUBTRACTEDFORCESOURCE_H
#define FOAM_SUBTRACTEDFORCESOURCE_H

#include "forcesources.h"

namespace Foam {

class subtractedForceSource
        : public forceSource
{
protected:
    forceSource *a_, *b_;

public:
    subtractedForceSource(const word& lbl, forceSource* a, forceSource* b, bool autoRegister=true);

    vector force() const override;
};

} // namespace Foam

#endif // FOAM_SUBTRACTEDFORCESOURCE_H
