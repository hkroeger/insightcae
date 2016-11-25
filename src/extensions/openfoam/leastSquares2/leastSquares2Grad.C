/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | foam-extend: Open Source CFD
   \\    /   O peration     |
    \\  /    A nd           | For copyright notice see file Copyright
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of foam-extend.

    foam-extend is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation, either version 3 of the License, or (at your
    option) any later version.

    foam-extend is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with foam-extend.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "leastSquares2Grad.H"
#include "leastSquares2Vectors.H"
#include "gaussGrad.H"
#include "fvMesh.H"
#include "volMesh.H"
#include "surfaceMesh.H"
#include "GeometricField.H"
#include "zeroGradientFvPatchField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
tmp
<
    GeometricField
    <
        typename outerProduct<vector, Type>::type, fvPatchField, volMesh
    >
>
leastSquares2Grad<Type>::
#if !(defined(OF16ext) && !defined(Fx40))
	calcGrad
#else
	grad
#endif
(
    const GeometricField<Type, fvPatchField, volMesh>& vsf
#if !(defined(OF16ext) && !defined(Fx40))
	    , const word& name
#endif
) const
{
    typedef typename outerProduct<vector, Type>::type GradType;

    const fvMesh& mesh = vsf.mesh();

    tmp<GeometricField<GradType, fvPatchField, volMesh> > tlsGrad
    (
        new GeometricField<GradType, fvPatchField, volMesh>
        (
            IOobject
            (
                "grad("+vsf.name()+')',
                vsf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<GradType>
            (
                "zero",
                vsf.dimensions()/dimLength,
                pTraits<GradType>::zero
            ),
            zeroGradientFvPatchField<GradType>::typeName
        )
    );
    GeometricField<GradType, fvPatchField, volMesh>& lsGrad = tlsGrad
#if defined(OFplus)||defined(OFdev)
    .ref
#endif
    ();

    // Get reference to least square vectors
    const leastSquares2Vectors& lsv = leastSquares2Vectors::New(mesh);

    const surfaceVectorField& ownLs = lsv.pVectors();
    const surfaceVectorField& neiLs = lsv.nVectors();

    const unallocLabelList& own = mesh.owner();
    const unallocLabelList& nei = mesh.neighbour();

    forAll(own, facei)
    {
        register label ownFaceI = own[facei];
        register label neiFaceI = nei[facei];

        Type deltaVsf = vsf[neiFaceI] - vsf[ownFaceI];

        lsGrad[ownFaceI] += ownLs[facei]*deltaVsf;
        lsGrad[neiFaceI] -= neiLs[facei]*deltaVsf;
    }

    // Boundary faces
    forAll(vsf.boundaryField(), patchi)
    {
        const fvsPatchVectorField& patchOwnLs = ownLs.boundaryField()[patchi];

        const unallocLabelList& faceCells =
            lsGrad.boundaryField()[patchi].patch().faceCells();

        if (vsf.boundaryField()[patchi].coupled())
        {
            Field<Type> neiVsf =
                vsf.boundaryField()[patchi].patchNeighbourField();

            forAll(neiVsf, patchFaceI)
            {
                lsGrad[faceCells[patchFaceI]] +=
                    patchOwnLs[patchFaceI]*
                    (neiVsf[patchFaceI] - vsf[faceCells[patchFaceI]]);
            }
        }
        else
        {
            const fvPatchField<Type>& patchVsf = vsf.boundaryField()[patchi];

            forAll(patchVsf, patchFaceI)
            {
                lsGrad[faceCells[patchFaceI]] +=
                     patchOwnLs[patchFaceI]*
                    (patchVsf[patchFaceI] - vsf[faceCells[patchFaceI]]);
            }
        }
    }


    lsGrad.correctBoundaryConditions();
    gaussGrad<Type>::correctBoundaryConditions(vsf, lsGrad);

    return tlsGrad;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
