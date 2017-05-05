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

#include "CICSAMDC.H"
#include "fvc.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "upwind.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

#if defined(OFplus)
#define UNALLOCLABELLIST labelList
#else
#define UNALLOCLABELLIST unallocLabelList
#endif

Foam::scalar Foam::CICSAMDC::weight
(
    const scalar cdWeight,
    const scalar faceFlux,
    const scalar& phiP,
    const scalar& phiN,
    const vector& gradcP,
    const vector& gradcN,
    const scalar Cof,
    const vector d
) const
{
    // Calculate upwind value, faceFlux C tilde and do a stabilisation

    scalar phict = 0;
    scalar phiupw = 0;
    scalar costheta = 0;

    if (faceFlux > 0)
    {
        costheta = mag((gradcP & d)/(mag(gradcP)*mag(d) + SMALL));

        phiupw = phiN - 2*(gradcP & d);

        phiupw = max(min(phiupw, 1.0), 0.0);

        if ((phiN - phiupw) > 0)
        {
            phict = (phiP - phiupw)/(phiN - phiupw + SMALL);
        }
        else
        {
            phict = (phiP - phiupw)/(phiN - phiupw - SMALL);
        }
    }
    else
    {
        costheta = mag((gradcN & d)/(mag(gradcN)*mag(d) + SMALL));

        phiupw = phiP + 2*(gradcN & d);

        phiupw = max(min(phiupw, 1.0), 0.0);

        if ((phiP - phiupw) > 0)
        {
            phict = (phiN - phiupw)/(phiP - phiupw + SMALL);
        }
        else
        {
            phict = (phiN - phiupw)/(phiP - phiupw - SMALL);
        }
    }


    // Calculate the weighting factors for CICSAMDC

    scalar cicsamFactor = (k_ + SMALL)/(1 - k_ + SMALL);

    costheta = min(1.0, cicsamFactor*(costheta));
    costheta = (cos(2*(acos(costheta))) + 1)/2;

    scalar k1 = (3*Cof*Cof - 3*Cof)/(2*Cof*Cof + 6*Cof - 8);
    scalar k2 = Cof;
    scalar k3 = (3*Cof + 5)/(2*Cof + 6);
    scalar weight;

    if (phict > 0 && phict <= k1)             // use blended scheme 1
    {
        scalar phifCM = phict/(Cof + SMALL);
        weight = (phifCM - phict)/(1 - phict);
    }
    else if (phict > k1 && phict <= k2)     // use blended scheme 2
    {
        scalar phifHC = phict/(Cof + SMALL);
        scalar phifUQ = (8*Cof*phict + (1 - Cof)*(6*phict + 3))/8;
        scalar phifCM = costheta*phifHC + (1 - costheta)*phifUQ;
        weight = (phifCM - phict)/(1 - phict);
    }
    else if (phict > k2 && phict < k3)     // use blended scheme 3
    {
        scalar phifUQ = (8*Cof*phict + (1 - Cof)*(6*phict + 3))/8;
        scalar phifCM = costheta + (1 - costheta)*phifUQ;
        weight = (phifCM - phict)/(1 - phict);
    }
    else if (phict >= k3 && phict <= 1)     // use downwind
    {
        weight = 1;
    }
    else                                    // use upwind
    {
        weight = 0;
    }

    if (faceFlux > 0)
    {
        return 1 - weight;
    }
    else
    {
        return weight;
    }
}


Foam::tmp<Foam::surfaceScalarField> Foam::CICSAMDC::correction
(
    const volScalarField& vf
) const
{
    const fvMesh& mesh = this->mesh();

    // Correction = full interpolation minus upwinded part
    tmp<surfaceScalarField> tcorr
    (
        new surfaceScalarField
        (
            IOobject
            (
                type() + "correction(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh
            ),
            mesh,
            vf.dimensions()
        )
    );
    surfaceScalarField& corr = tcorr
#if defined(OFplus)||defined(OFdev)
    .ref
#endif
    ();

    volVectorField gradc = fvc::grad(vf);

    surfaceScalarField Cof =
        mesh.time().deltaT()
       *upwind<scalar>(mesh, faceFlux_).interpolate
        (
            fvc::surfaceIntegrate(faceFlux_)
        );

    const surfaceScalarField& CDweights = mesh.surfaceInterpolation::weights();

    const UNALLOCLABELLIST& owner = mesh.owner();
    const UNALLOCLABELLIST& neighbour = mesh.neighbour();

    const vectorField& C = mesh.C();

    scalarField& corrIn = corr
#if defined(OFdev)||defined(OFplus)
      .ref().field()
#else
      .internalField()
#endif
      ;

    scalar w;

    forAll(corrIn, faceI)
    {
        label own = owner[faceI];
        label nei = neighbour[faceI];

        w = weight
        (
            CDweights[faceI],
            this->faceFlux_[faceI],
            vf[own],
            vf[nei],
            gradc[own],
            gradc[nei],
            Cof[faceI],
            C[nei] - C[own]
        );

        corrIn[faceI] = w*vf[own] + (1 - w)*vf[nei];
    }

#if defined(OFdev)||defined(OFplus)
    surfaceScalarField::Boundary& 
#else
    surfaceScalarField::GeometricBoundaryField& 
#endif
      bCorr = corr
#if defined(OFdev)||defined(OFplus)
	.boundaryFieldRef()
#else
	.boundaryField()
#endif
	;

#if defined(OFdev)||defined(OFplus)
    surfaceScalarField::Boundary& 
#else
    surfaceScalarField::GeometricBoundaryField& 
#endif
      bCof = Cof
#if defined(OFdev)||defined(OFplus)
	.boundaryFieldRef()
#else
	.boundaryField()
#endif
	;

    forAll(bCorr, patchI)
    {
        scalarField& pCorr = bCorr[patchI];

        if (bCorr[patchI].coupled())
        {
            const scalarField& pCDweights = CDweights.boundaryField()[patchI];

            const scalarField& pFaceFlux =
                this->faceFlux_.boundaryField()[patchI];

            scalarField pvfP =
                vf.boundaryField()[patchI].patchInternalField();

            scalarField pvfN =
                vf.boundaryField()[patchI].patchNeighbourField();

            vectorField pGradcP =
                gradc.boundaryField()[patchI].patchInternalField();

            vectorField pGradcN =
                gradc.boundaryField()[patchI].patchNeighbourField();

            const scalarField& pCof = Cof.boundaryField()[patchI];

            // Build the d-vectors
            // Better version of d-vectors: Zeljko Tukovic, 25/Apr/2010
            vectorField pd = bCorr[patchI].patch().delta();

            scalar w;

            forAll(pCorr, faceI)
            {
                w = weight
                (
                    pCDweights[faceI],
                    pFaceFlux[faceI],
                    pvfP[faceI],
                    pvfN[faceI],
                    pGradcP[faceI],
                    pGradcN[faceI],
                    pCof[faceI],
                    pd[faceI]
                );

                pCorr[faceI] = w*pvfP[faceI] + (1 - w)*pvfN[faceI];
            }
        }
        else
        {
            pCorr = vf.boundaryField()[patchI];
        }
    }

    return tcorr;
}


namespace Foam
{
//defineNamedTemplateTypeNameAndDebug(CICSAMDC, 0);
defineTypeNameAndDebug(CICSAMDC, 0);

surfaceInterpolationScheme<scalar>::addMeshConstructorToTable<CICSAMDC>
    addCICSAMDCMeshConstructorToTable_;

surfaceInterpolationScheme<scalar>::addMeshFluxConstructorToTable<CICSAMDC>
    addCICSAMDCMeshFluxConstructorToTable_;

limitedSurfaceInterpolationScheme<scalar>::addMeshConstructorToTable<CICSAMDC>
    addCICSAMDCMeshConstructorToLimitedTable_;

limitedSurfaceInterpolationScheme<scalar>::
addMeshFluxConstructorToTable<CICSAMDC>
    addCICSAMDMeshFluxConstructorToLimitedTable_;
}

// ************************************************************************* //
