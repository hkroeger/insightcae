/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "localFaceLimitedGrad.H"
#include "gaussGrad.H"
#include "fvMesh.H"
#include "volMesh.H"
#include "surfaceMesh.H"
#include "volFields.H"
#include "fixedValueFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#if (OF_VERSION<030000) //not (defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806))
namespace Foam
{
namespace fv
{
#endif
  
    makeFvGradScheme(localFaceLimitedGrad)

#if (OF_VERSION<030000) //not (defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806))
}
}
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


template<>
Foam::tmp<Foam::volVectorField>
Foam::fv::localFaceLimitedGrad<Foam::scalar>::
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
grad
#else
calcGrad
#endif
(
    const volScalarField& vsf
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
#else
    ,
    const word& name
#endif
) const
{
    const fvMesh& mesh = vsf.mesh();

    tmp<volVectorField> tGrad = basicGradScheme_().
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
    grad(vsf);
#else
    calcGrad(vsf, name);
#endif
    
//     if (k_ < SMALL)
//     {
//         return tGrad;
//     }

    volVectorField& g = UNIOF_TMP_NONCONST(tGrad);

    const UNIOF_LABELULIST& owner = mesh.owner();
    const UNIOF_LABELULIST& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    // create limiter
    scalarField limiter(vsf.internalField().size(), 1.0);

    surfaceScalarField rk = (1.0/getk(mesh) - 1.0);

    forAll(owner, facei)
    {
        label own = owner[facei];
        label nei = neighbour[facei];

        scalar vsfOwn = vsf[own];
        scalar vsfNei = vsf[nei];

        scalar maxFace = max(vsfOwn, vsfNei);
        scalar minFace = min(vsfOwn, vsfNei);
        scalar maxMinFace = rk[facei]*(maxFace - minFace);
        maxFace += maxMinFace;
        minFace -= maxMinFace;

        // owner side
        limitFace
        (
            limiter[own],
            maxFace - vsfOwn, minFace - vsfOwn,
            (Cf[facei] - C[own]) & g[own]
        );

        // neighbour side
        limitFace
        (
            limiter[nei],
            maxFace - vsfNei, minFace - vsfNei,
            (Cf[facei] - C[nei]) & g[nei]
        );
    }

#if (OF_VERSION>=040000) //defined(OFplus)||defined(OFdev)||defined(OFesi1806)
    const volScalarField::Boundary& 
#else
    const volScalarField::GeometricBoundaryField& 
#endif
      bsf = vsf.boundaryField();

    forAll(bsf, patchi)
    {
        const fvPatchScalarField& psf = bsf[patchi];

        const UNIOF_LABELULIST& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        if (psf.coupled())
        {
            const scalarField psfNei(psf.patchNeighbourField());

            forAll(pOwner, pFacei)
            {
                label own = pOwner[pFacei];

                scalar vsfOwn = vsf[own];
                scalar vsfNei = psfNei[pFacei];

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);
                scalar maxMinFace = rk[pFacei]*(maxFace - minFace);
                maxFace += maxMinFace;
                minFace -= maxMinFace;

                limitFace
                (
                    limiter[own],
                    maxFace - vsfOwn, minFace - vsfOwn,
                    (pCf[pFacei] - C[own]) & g[own]
                );
            }
        }
        else if (psf.fixesValue())
        {
            forAll(pOwner, pFacei)
            {
                label own = pOwner[pFacei];

                scalar vsfOwn = vsf[own];
                scalar vsfNei = psf[pFacei];

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);
                scalar maxMinFace = rk[pFacei]*(maxFace - minFace);
                maxFace += maxMinFace;
                minFace -= maxMinFace;

                limitFace
                (
                    limiter[own],
                    maxFace - vsfOwn, minFace - vsfOwn,
                    (pCf[pFacei] - C[own]) & g[own]
                );
            }
        }
    }

    if (fv::debug)
    {
        Info<< "gradient limiter for: " << vsf.name()
            << " max = " << gMax(limiter)
            << " min = " << gMin(limiter)
            << " average: " << gAverage(limiter) << endl;
    }

    UNIOF_INTERNALFIELD_NONCONST(g)  *= limiter;

    g.correctBoundaryConditions();
    gaussGrad<scalar>::correctBoundaryConditions(vsf, g);

    return tGrad;
}


template<>
Foam::tmp<Foam::volTensorField>
Foam::fv::localFaceLimitedGrad<Foam::vector>::
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
grad
#else
calcGrad
#endif
(
    const volVectorField& vvf
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
#else
    ,
    const word& name
#endif
) const
{
    const fvMesh& mesh = vvf.mesh();

    tmp<volTensorField> tGrad = basicGradScheme_().
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
    grad(vvf);
#else
    calcGrad(vvf, name);
#endif
    
//     if (k_ < SMALL)
//     {
//         return tGrad;
//     }

    volTensorField& g = UNIOF_TMP_NONCONST(tGrad);

    const UNIOF_LABELULIST& owner = mesh.owner();
    const UNIOF_LABELULIST& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    // create limiter
    scalarField limiter(vvf.internalField().size(), 1.0);

    surfaceScalarField rk = (1.0/getk(mesh) - 1.0);

    forAll(owner, facei)
    {
        label own = owner[facei];
        label nei = neighbour[facei];

        vector vvfOwn = vvf[own];
        vector vvfNei = vvf[nei];

        // owner side
        vector gradf = (Cf[facei] - C[own]) & g[own];

        scalar vsfOwn = gradf & vvfOwn;
        scalar vsfNei = gradf & vvfNei;

        scalar maxFace = max(vsfOwn, vsfNei);
        scalar minFace = min(vsfOwn, vsfNei);
        scalar maxMinFace = rk[facei]*(maxFace - minFace);
        maxFace += maxMinFace;
        minFace -= maxMinFace;

        limitFace
        (
            limiter[own],
            maxFace - vsfOwn, minFace - vsfOwn,
            magSqr(gradf)
        );


        // neighbour side
        gradf = (Cf[facei] - C[nei]) & g[nei];

        vsfOwn = gradf & vvfOwn;
        vsfNei = gradf & vvfNei;

        maxFace = max(vsfOwn, vsfNei);
        minFace = min(vsfOwn, vsfNei);

        limitFace
        (
            limiter[nei],
            maxFace - vsfNei, minFace - vsfNei,
            magSqr(gradf)
        );
    }

#if (OF_VERSION>=040000) //defined(OFdev)||defined(OFplus)||defined(OFesi1806)
    const volVectorField::Boundary&
#else
    const volVectorField::GeometricBoundaryField&
#endif
      bvf = vvf.boundaryField();

    forAll(bvf, patchi)
    {
        const fvPatchVectorField& psf = bvf[patchi];

        const UNIOF_LABELULIST& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        if (psf.coupled())
        {
            const vectorField psfNei(psf.patchNeighbourField());

            forAll(pOwner, pFacei)
            {
                label own = pOwner[pFacei];

                vector vvfOwn = vvf[own];
                vector vvfNei = psfNei[pFacei];

                vector gradf = (pCf[pFacei] - C[own]) & g[own];

                scalar vsfOwn = gradf & vvfOwn;
                scalar vsfNei = gradf & vvfNei;

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);
                scalar maxMinFace = rk[pFacei]*(maxFace - minFace);
                maxFace += maxMinFace;
                minFace -= maxMinFace;

                limitFace
                (
                    limiter[own],
                    maxFace - vsfOwn, minFace - vsfOwn,
                    magSqr(gradf)
                );
            }
        }
        else if (psf.fixesValue())
        {
            forAll(pOwner, pFacei)
            {
                label own = pOwner[pFacei];

                vector vvfOwn = vvf[own];
                vector vvfNei = psf[pFacei];

                vector gradf = (pCf[pFacei] - C[own]) & g[own];

                scalar vsfOwn = gradf & vvfOwn;
                scalar vsfNei = gradf & vvfNei;

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);
                scalar maxMinFace = rk[pFacei]*(maxFace - minFace);
                maxFace += maxMinFace;
                minFace -= maxMinFace;

                limitFace
                (
                    limiter[own],
                    maxFace - vsfOwn, minFace - vsfOwn,
                    magSqr(gradf)
                );
            }
        }
    }

    if (fv::debug)
    {
        Info<< "gradient limiter for: " << vvf.name()
            << " max = " << gMax(limiter)
            << " min = " << gMin(limiter)
            << " average: " << gAverage(limiter) << endl;
    }

    UNIOF_INTERNALFIELD_NONCONST(g) *= limiter;
    g.correctBoundaryConditions();
    gaussGrad<vector>::correctBoundaryConditions(vvf, g);

    return tGrad;
}


// ************************************************************************* //
