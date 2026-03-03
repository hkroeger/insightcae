#ifndef WALLDISTANCE_H
#define WALLDISTANCE_H

#include "uniof_functionobject.h"
#include "fvMesh.H"

#include "wallDist.H"

namespace Foam {


class wallDistance
    : public UniFunctionObject
{
private:
    const fvMesh& mesh_;

    wallDist wd_;
    volScalarField wallDist_;
    volVectorField wallDistGrad_;

public:
    TypeName("wallDistance");

    wallDistance(
        const word& name,
        const objectRegistry& obr,
        const dictionary& dict );

    bool perform() override;
    bool write() override;
};


} // namespace Foam

#endif
