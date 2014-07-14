/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\*---------------------------------------------------------------------------*/

#include "reconCentral.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh> >
Foam::reconCentral<Type>::interpolate
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh> > tsf
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "interpolate("+vf.name()+')',
                vf.instance(),
                vf.db()
            ),
            mesh,
            dimensioned<Type>(vf.name(), vf.dimensions(), pTraits<Type>::zero)
        )
    );

    GeometricField<Type, fvsPatchField, surfaceMesh>& sf = tsf();

    const labelList& owner = mesh.owner();
    const labelList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    GeometricField
        <typename outerProduct<vector, Type>::type, fvPatchField, volMesh>
        gradVf = gradScheme_().grad(vf);

    Field<Type>& sfIn = sf.internalField();

    forAll(sfIn, facei)
    {
        // Owner contribution
        label own = owner[facei];
        sfIn[facei] += 0.5*(vf[own] + ((Cf[facei] - C[own]) & gradVf[own]));

        // Neighbour contribution
        label nei = neighbour[facei];
        sfIn[facei] += 0.5*(vf[nei] + ((Cf[facei] - C[nei]) & gradVf[nei]));
    }


    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
        GeometricBoundaryField& bSf = sf.boundaryField();

    forAll(bSf, patchi)
    {
        const fvPatch& p = mesh.boundary()[patchi];

        fvsPatchField<Type>& pSf = bSf[patchi];

        const unallocLabelList& pOwner = p.faceCells();

        const vectorField& pCf = Cf.boundaryField()[patchi];

        if (pSf.coupled())
        {
            Field<Type> vfNei =
                vf.boundaryField()[patchi].patchNeighbourField();

            Field<typename outerProduct<vector, Type>::type> pGradVfNei =
                gradVf.boundaryField()[patchi].patchNeighbourField();

            // Build the d-vectors.  Used to calculate neighbour face centre
            // HJ, 19/Apr/2010
            // Better version of d-vectors: Zeljko Tukovic, 25/Apr/2010
            vectorField pd = p.delta();

            forAll(pOwner, facei)
            {
                label own = pOwner[facei];

                // Owner contribution
                pSf[facei] +=
                    0.5*(vf[own] + ((pCf[facei] - C[own]) & gradVf[own]));

                // Neighbour contribution
                pSf[facei] +=
                    0.5*
                    (
                        vfNei[facei]
                      + (
                            (pCf[facei] - pd[facei] - C[own])
                          & pGradVfNei[facei]
                        )
                    );
            }
        }
        else if (vf.boundaryField()[patchi].fixesValue())
        {
            // For fixed boundary patches copy the value
            pSf = vf.boundaryField()[patchi];
        }
        else
        {
            // For patches that do not fix the value, calculate
            // extrapolated field
            forAll(pOwner, facei)
            {
                label own = pOwner[facei];

                pSf[facei] =
                    (vf[own] + ((pCf[facei] - C[own]) & gradVf[own]));
            }
        }
    }

    return tsf;
}


template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh> >
Foam::reconCentral<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    // Note: Correction is calculated by assembling the complete interpolation
    // including extrapolated gradient contribution and subtracting the
    // implicit contribution.  HJ, 27/Mar/2010
    const fvMesh& mesh = this->mesh();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh> > tsfCorr
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "reconCentralCorrection(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                false
            ),
            reconCentral<Type>::interpolate(vf)
          - surfaceInterpolationScheme<Type>::interpolate
            (
                vf,
                this->weights()
            )
        )
    );

    return tsfCorr;
}


namespace Foam
{
    //makelimitedSurfaceInterpolationScheme(reconCentral)
    makelimitedSurfaceInterpolationTypeScheme(reconCentral, scalar)
    makelimitedSurfaceInterpolationTypeScheme(reconCentral, vector)
}

// ************************************************************************* //
