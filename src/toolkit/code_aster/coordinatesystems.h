#ifndef COORDINATESYSTEMS_H
#define COORDINATESYSTEMS_H

#include "base/spatialtransformation.h"

namespace insight
{

/**
 * @brief The BeamLocalCS class
 * is a spatial transformation from the local CA beam coordinate system
 * back to the global CS.
 */
class BeamLocalCS
        : public SpatialTransformation
{
public:
    /**
     * @brief BeamLocalCS
     * constructor
     * @param origin
     * the point on the beam axis in global coordinates
     * @param alongBeamDir
     * the direction along the beam axis in global coordinates
     */
    BeamLocalCS(const arma::mat& origin, const arma::mat& alongBeamDir);

    SpatialTransformation MACRCARAPOUTRE() const;
};


}

#endif // COORDINATESYSTEMS_H
