#ifndef FOAM_GLOBALCOORDINATESYSTEMS_H
#define FOAM_GLOBALCOORDINATESYSTEMS_H

#include "globalobject.h"
#include "globalregistry.h"

#include "coordinateSystem.H"


namespace Foam {

class coordinateSystemSource;



typedef globalRegistry<coordinateSystemSource> coordinateSystemSources;



class coordinateSystemSource
        : public globalObject<coordinateSystemSources>
{
public:
    coordinateSystemSource(const word& lbl, bool autoRegister=true);

    virtual autoPtr<coordinateSystem> getCoordinateSystem() const =0;
};


class globalCoordinateSystem
        : public coordinateSystemSource
{
    globalCoordinateSystem();

public:
    static globalCoordinateSystem theGlobalCoordinateSystem;

    autoPtr<coordinateSystem> getCoordinateSystem() const override;
};

} // namespace Foam

#endif // FOAM_GLOBALCOORDINATESYSTEMS_H
