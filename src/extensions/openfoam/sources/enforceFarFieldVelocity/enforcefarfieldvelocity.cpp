
#include "enforcefarfieldvelocity.h"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{
namespace fv
{


enforceFarFieldVelocity::enforceFarFieldVelocity
(
        const word& name,
        const word& modelType,
        const dictionary& dict,
        const fvMesh& mesh
        )
    : option(name, modelType, dict, mesh),
      farFieldPatches_(dict.lookup("farFieldPatches")),
      y_(
          IOobject
          (
              "yEnforce",
              mesh.time().timeName(),
              mesh
          ),
          mesh,
          dimensionedScalar("yEnforce", dimLength, SMALL),
          patchDistMethod::patchTypes<scalar>(
              mesh,
              mesh.boundaryMesh().patchSet(
                  farFieldPatches_
                  ))
      ),
      UFarField_(nullptr)
{
    enforceFarFieldVelocity::read(dict);
}

//- Destructor
enforceFarFieldVelocity::~enforceFarFieldVelocity()
{
}


// Member Functions

//- Read dictionary
bool enforceFarFieldVelocity::read(const dictionary& dict)
{
    farFieldVelocityFieldName_=word(coeffs_.lookup("farFieldVelocityFieldName"));
    farFieldPatches_=wordReList(dict.lookup("farFieldPatches"));
    transitionDistance_=readLabel(dict.lookup("transitionDistance"));

    ym_.reset(patchDistMethod::New(
                 dict.subDict("patchDist"),
                 mesh_,
                 mesh_.boundaryMesh().patchSet(
                     farFieldPatches_
                     ) ) );
    if (farFieldVelocityFieldName_=="zero")
    {
        zero_.reset(new volVectorField(
                        IOobject
                        (
                            "zero",
                            mesh_.time().timeName(),
                            mesh_
                            ),
                        mesh_,
                        dimensionedVector("zero", dimVelocity, vector::zero),
                        "calculated"));
        UFarField_ = zero_.get();
    }
    else
    {
        UFarField_ = &mesh_.lookupObject<volVectorField>(farFieldVelocityFieldName_);
    }

    word UName(dict.lookupOrDefault<word>("U", "U"));
    fieldNames_.resize(1, UName);
    applied_.setSize(fieldNames_.size(), false);

    return true;
}


//- Correct the velocity field
void enforceFarFieldVelocity::correct(volVectorField& U)
{
    ym_->correct(y_);

    dimensionedScalar td("", dimLength, transitionDistance_);

    volScalarField x(
                1. -  y_ / td );

    vectorField& Uif = U.primitiveFieldRef();

    forAll(x, celli)
    {
        scalar xi=x[celli];

        if (xi>=0 && xi<=1.)
        {
            Uif[celli] =
                    xi*(*UFarField_)[celli]
                    + (1.-xi)*Uif[celli];
        }
    }

    // handle boundaries in the case of 'all'
    typename volVectorField::Boundary& Ubf =
            U.boundaryFieldRef();

    forAll(Ubf, patchi)
    {
        fvPatchVectorField& Up = Ubf[patchi];

        if (!Up.fixesValue())
        {
            forAll(Up, facei)
            {
                scalar xi =
                        x.boundaryField()[patchi][facei];

                if (xi>=0 && xi<=1.)
                {
                    Up[facei] =
                            xi*UFarField_->boundaryField()[patchi][facei]
                            + (1.-xi)*Up[facei];
                }
            }
        }
    }

    U.correctBoundaryConditions();
}

defineTypeNameAndDebug(enforceFarFieldVelocity, 0);
addToRunTimeSelectionTable
(
    option,
    enforceFarFieldVelocity,
    dictionary
);


}
}
