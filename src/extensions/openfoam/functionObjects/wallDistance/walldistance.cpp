#include "walldistance.h"
#include "addToRunTimeSelectionTable.H"

#include "fvcGrad.H"

namespace Foam {



defineTypeNameAndDebug(wallDistance, 0);

addToRunTimeSelectionTable(
    functionObject,
    wallDistance,
    dictionary
    );



wallDistance::wallDistance(
    const word& name,
    const objectRegistry& obr,
    const dictionary& dict )
  : UniFunctionObject(name, dict),
    mesh_(UNIOF_OBR_TO_MESH(obr)),
    wd_(mesh_),
    wallDist_(
        IOobject(
            "wallDistance",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
            ),
          mesh_,
        dimensionedScalar("defl", dimLength, 0.)
        ),
    wallDistGrad_(
        IOobject(
            "wallDistanceGradient",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
            ),
        mesh_,
        dimensionedVector("defl", dimless, vector::zero)
        )
{}



bool wallDistance::perform()
{
    wallDist_=wd_.y();
    wallDistGrad_=fvc::grad(wallDist_);
    return true;
}

bool wallDistance::write()
{
    wallDist_.write();
    wallDistGrad_.write();
    return true;
}

}
