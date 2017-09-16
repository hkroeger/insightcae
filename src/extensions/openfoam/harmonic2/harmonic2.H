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

Class
    Foam::harmonic2

Description
    Harmonic-mean differencing scheme class.

    This scheme interpolates 1/field using a scheme specified at run-time
    and return the reciprocal of the interpolate.

SourceFiles
    harmonic2.C

\*---------------------------------------------------------------------------*/

#ifndef harmonic2_H
#define harmonic2_H

#include "surfaceInterpolationScheme.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#if defined(OFplus)
#define UNALLOCLABELLIST labelList
#else
#define UNALLOCLABELLIST unallocLabelList
#endif

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class harmonic2 Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class harmonic2
:
    public surfaceInterpolationScheme<Type>
{
    // Private Member Functions

        //- Disallow default bitwise assignment
        void operator=(const harmonic2&);


public:

    //- Runtime type information
    TypeName("harmonic2");


    // Constructors

        //- Construct from mesh
        harmonic2(const fvMesh& mesh)
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}

        //- Construct from Istream
        //  The name of the flux field is read from the Istream and looked-up
        //  from the mesh objectRegistry
        harmonic2
        (
            const fvMesh& mesh,
            Istream& is
        )
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}

        //- Construct from faceFlux and Istream
        harmonic2
        (
            const fvMesh& mesh,
            const surfaceScalarField& faceFlux,
            Istream& is
        )
        :
            surfaceInterpolationScheme<Type>(mesh)
        {}


    // Member Functions

        //- Return the interpolation weighting factors
        virtual tmp<surfaceScalarField> weights
        (
            const GeometricField<Type, fvPatchField, volMesh>& phi
        ) const
        {
            tmp<surfaceScalarField> tw
            (
                new surfaceScalarField
                (
                    IOobject
                    (
                        "harmonic2WeightingFactors" + phi.name(),
                        this->mesh().time().timeName(),
                        this->mesh()
                    ),
                    this->mesh() ,
                    dimless
                )
            );

            surfaceScalarField& w = tw
#if defined(OFplus)||defined(OFdev)
	    .ref
#endif
            ();

            const UNALLOCLABELLIST& owner = this->mesh().owner();
            const UNALLOCLABELLIST& neighbour = this->mesh().neighbour();

            scalarField magPhi = mag(phi);

            scalarField& wIn = w
#if defined(OFdev)||defined(OFplus)
	    .ref().field()
#else
            .internalField()
#endif
	    ;

            // Calculate internal weights using field magnitude
            forAll (owner, faceI)
            {
                scalar mOwn = magPhi[owner[faceI]];
                scalar mNei = magPhi[neighbour[faceI]];

                scalar den = mOwn - mNei;

                if (mag(den) > SMALL)
                {
                    scalar mean = 2.0*mOwn*mNei/(mOwn + mNei);
                    wIn[faceI] = (mean - mNei)/den;
                }
                else
                {
                    wIn[faceI] = 0.5;
                }
            }

            forAll (phi.boundaryField(), pi)
            {
                fvsPatchScalarField& wp = w
#if defined(OFdev)||defined(OFplus)
                .boundaryFieldRef()
#else
                .boundaryField()
#endif
		[pi];

                const fvPatchField<Type>& patchPhi = phi.boundaryField()[pi];

                if (patchPhi.coupled())
                {
                    scalarField magPhiOwn = mag(patchPhi.patchInternalField());
                    scalarField magPhiNei = mag(patchPhi.patchNeighbourField());

                    // Calculate internal weights using field magnitude
                    forAll (patchPhi, faceI)
                    {
                        scalar mOwn = magPhiOwn[faceI];
                        scalar mNei = magPhiNei[faceI];

                        scalar den = mOwn - mNei;

                        if (mag(den) > SMALL)
                        {
                             scalar mean = 2.0*mOwn*mNei/(mOwn + mNei);
                             wp[faceI] = (mean - mNei)/den;
                        }
                        else
                        {
                             wp[faceI] = 0.5;
                        }
                    }
                }
                else
                {
                    // Boundary weights for uncoupled patches are 1
                    wp = 1;
                }
            }

            return tw;
        }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
