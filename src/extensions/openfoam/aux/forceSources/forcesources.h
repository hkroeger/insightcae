#ifndef FOAM_FORCESOURCES_H
#define FOAM_FORCESOURCES_H


#include "globalobject.h"
#include "globalregistry.h"

#include "vector.H"

namespace Foam {




class forceSource;



typedef globalRegistry<forceSource> forceSources;



class forceSource
        : public globalObject<forceSources>
{
public:
    forceSource(const word& lbl, bool autoRegister=true);

    virtual vector force() const =0;
};


} // namespace Foam

#endif // FOAM_FORCESOURCES_H
