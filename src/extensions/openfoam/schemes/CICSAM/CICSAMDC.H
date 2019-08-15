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
    Foam::CICSAMDC

Description
    linear/upwind CICSAM differencing scheme in deferred correction form.

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved

SourceFiles
    CICSAMDC.C

\*---------------------------------------------------------------------------*/

#ifndef CICSAMDC_H
#define CICSAMDC_H

#include "upwind.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class CICSAMDC Declaration
\*---------------------------------------------------------------------------*/

class CICSAMDC
:
    public upwind<scalar>
{
    // Private data

        //- Scheme factor
        const scalar k_;


    // Private Member Functions

        //- Disallow default bitwise copy construct
        CICSAMDC(const CICSAMDC&);

        //- Disallow default bitwise assignment
        void operator=(const CICSAMDC&);


        //- Calculate individual weight
        scalar weight
        (
            const scalar cdWeight,
            const scalar faceFlux,
            const scalar& phiP,
            const scalar& phiN,
            const vector& gradcP,
            const vector& gradcN,
            const scalar Cof,
            const vector d
        ) const;

public:

    //- Runtime type information
    TypeName("CICSAMDC");


    // Constructors

        //- Construct from mesh, faceFlux and blendingFactor
        CICSAMDC
        (
            const fvMesh& mesh,
            const surfaceScalarField& faceFlux,
            const scalar k
        )
        :
            upwind<scalar>(mesh, faceFlux),
            k_(k)
        {}

        //- Construct from mesh and Istream.
        //  The name of the flux field is read from the Istream and looked-up
        //  from the mesh objectRegistry
        CICSAMDC
        (
            const fvMesh& mesh,
            Istream& is
        )
        :
            upwind<scalar>(mesh, is),
            k_(readScalar(is))
        {
            if (k_ < 0 || k_ > 1)
            {
                FatalIOErrorIn("CICSAMDCWeight(Istream& is)", is)
                    << "coefficient = " << k_
                    << " should be >= 0 and <= 1"
                    << exit(FatalIOError);
            }
        }

        //- Construct from mesh, faceFlux and Istream
        CICSAMDC
        (
            const fvMesh& mesh,
            const surfaceScalarField& faceFlux,
            Istream& is
        )
        :
            upwind<scalar>(mesh, faceFlux),
            k_(readScalar(is))
        {
            if (k_ < 0 || k_ > 1)
            {
                FatalIOErrorIn("CICSAMDCWeight(Istream& is)", is)
                    << "coefficient = " << k_
                    << " should be >= 0 and <= 1"
                    << exit(FatalIOError);
            }
        }


    // Member Functions

        //- Return true if this scheme uses an explicit correction
        virtual bool corrected() const
        {
            return true;
        }

        //- Return the explicit correction to the face-interpolate
        virtual tmp<surfaceScalarField>
        correction
        (
            const volScalarField& vf
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
