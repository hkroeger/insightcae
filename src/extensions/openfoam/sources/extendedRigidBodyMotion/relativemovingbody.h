#ifndef FOAM_RELATIVEMOVINGBODY_H
#define FOAM_RELATIVEMOVINGBODY_H

#include "movingmeshpart.h"
#include "solidBodyMotionFunction.H"

namespace Foam {

struct relativeMovingBody
    : public IndependentMovingMeshPart
{
    autoPtr<solidBodyMotionFunction> smbf_;

    relativeMovingBody(const dictionary& dict, const polyMesh& mesh);

    septernion transformation() const override;

    autoPtr<relativeMovingBody> clone() const;
};

} // namespace Foam

#endif // FOAM_RELATIVEMOVINGBODY_H
