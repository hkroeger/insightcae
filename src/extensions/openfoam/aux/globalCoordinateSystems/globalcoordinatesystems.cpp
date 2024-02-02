#include "globalcoordinatesystems.h"

#if OF_VERSION>=010700
#include "cartesianCS.H"
#endif

namespace Foam {

coordinateSystemSource::coordinateSystemSource(const word& lbl, bool autoRegister)
    : globalObject<coordinateSystemSources>(lbl, autoRegister)
{}



globalCoordinateSystem::globalCoordinateSystem()
    : coordinateSystemSource("global")
{}

autoPtr<coordinateSystem> globalCoordinateSystem::getCoordinateSystem() const
{
    return autoPtr<coordinateSystem>(
                new
#if OF_VERSION>010700
                cartesianCS(
                    lbl_,
                    point::zero,
                    vector(0,0,1),
                    vector(1,0,0)
                    )
#else
                coordinateSystem(
                    lbl_,
                    point::zero,
                    vector(0,0,1),
                    vector(1,0,0)
                    )
#endif
                );
}


globalCoordinateSystem globalCoordinateSystem::theGlobalCoordinateSystem;


} // namespace Foam
