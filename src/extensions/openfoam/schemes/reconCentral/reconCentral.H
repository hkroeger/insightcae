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
    Foam::reconCentral

Description
    reconCentral interpolation scheme uses extrapolated gradient-based
    correction from both sides onto the face, using 1/2 weighting.
    Class is derived from upwind and returns upwind weighting factors.
    Correction is calculated using the extrapolated gradients.

Author
    Hrvoje Jasak, Wikki Ltd.  All rights reserved.

SourceFiles
    reconCentral.C

\*---------------------------------------------------------------------------*/

#ifndef reconCentral_H
#define reconCentral_H

#include "upwind.H"
#include "gaussGrad.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class reconCentral Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class reconCentral
:
    public upwind<Type>
{
    // Private Data

        tmp<fv::gradScheme<Type> > gradScheme_;


    // Private Member Functions

        //- Disallow default bitwise copy construct
        reconCentral(const reconCentral&);

        //- Disallow default bitwise assignment
        void operator=(const reconCentral&);


public:

    //- Runtime type information
    TypeName("reconCentral");


    // Constructors

        //- Construct from faceFlux
        reconCentral
        (
            const fvMesh& mesh,
            const surfaceScalarField& faceFlux
        )
        :
            upwind<Type>(mesh, faceFlux),
            gradScheme_
            (
                new fv::gaussGrad<Type>(mesh)
            )
        {}

        //- Construct from Istream.
        //  The name of the flux field is read from the Istream and looked-up
        //  from the mesh objectRegistry
        reconCentral
        (
            const fvMesh& mesh,
            Istream& schemeData
        )
        :
            upwind<Type>(mesh, schemeData),
            gradScheme_
            (
                fv::gradScheme<Type>::New
                (
                    mesh,
                    schemeData
                )
            )
        {}

        //- Construct from faceFlux and Istream
        reconCentral
        (
            const fvMesh& mesh,
            const surfaceScalarField& faceFlux,
            Istream& schemeData
        )
        :
            upwind<Type>(mesh, faceFlux, schemeData),
            gradScheme_
            (
                fv::gradScheme<Type>::New
                (
                    mesh,
                    schemeData
                )
            )
        {}


    // Member Functions

        //- Return the face-interpolate of the given cell field
        //  with explicit correction
        virtual tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
        interpolate(const GeometricField<Type, fvPatchField, volMesh>&) const;

        //- Return true if this scheme uses an explicit correction
        virtual bool corrected() const
        {
            return true;
        }

        //- Return the explicit correction to the face-interpolate
        virtual tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
        correction
        (
            const GeometricField<Type, fvPatchField, volMesh>& vf
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
