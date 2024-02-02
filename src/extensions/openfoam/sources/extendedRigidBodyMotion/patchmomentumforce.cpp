#include "patchmomentumforce.h"

#include "fvCFD.H"

namespace Foam {
namespace extRBM {


defineTypeNameAndDebug(patchMomentumForce, 0);


patchMomentumForce::patchMomentumForce
(
    const fvMesh& mesh,
    const word& patchName,
    scalar rhoInf,
    const word &rhoName
)
: forceSource(patchName, true),
  mesh_(mesh),
  patchName_(patchName),
  rhoInf_(rhoInf),
  rhoName_(rhoName),
  F_(vector::zero), M_(vector::zero)
{}




void patchMomentumForce::calcFM()
{
    const volVectorField& U =
            mesh_.lookupObject<volVectorField>("U");
    const volScalarField& p =
            mesh_.lookupObject<volScalarField>("p");

    Foam::tmp<Foam::volScalarField> rho;

    {
        if (rhoName_ == "rhoInf")
        {
            rho=tmp<volScalarField>
            (
                new volScalarField
                (
                    IOobject
                    (
                        "rho",
                        mesh_.time().timeName(),
                        mesh_
                    ),
                    mesh_,
                    dimensionedScalar("rho", dimDensity, rhoInf_)
                )
            );
        }
        else
        {
            rho=tmp<volScalarField>(
                        new volScalarField(
                            mesh_.lookupObject<volScalarField>(
                                rhoName_) ) );
        }
    }

    label id=mesh_.boundaryMesh().findPatchID(patchName_);

    const scalarField& rhop = rho().boundaryField()[id];
    const vectorField& Up = U.boundaryField()[id];
    const scalarField& pp = p.boundaryField()[id];
    const vectorField& Ap = mesh_.Sf().boundaryField()[id];


    vectorField f(
      rhop * Up * (Up & Ap)
      +
      pp*Ap
      );

    F_ = gSum( f );
    M_ = gSum( mesh_.Cf().boundaryField()[id] ^ f );

    Info << "[" << patchName_ << "] : F="<<F_<<", M="<<M_<<endl;
}




vector patchMomentumForce::force() const
{
    const_cast<patchMomentumForce*>(this)->calcFM();
    return F_;
}




std::pair<vector,vector>
patchMomentumForce::forceAndMoment() const
{
    const_cast<patchMomentumForce*>(this)->calcFM();
    return {F_, M_};
}


} // namespace extRBM
} // namespace Foam
