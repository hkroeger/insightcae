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

Class
    Foam::fv::leastSquares2Grad

Description
    Second-order gradient scheme using least-squares.

SourceFiles
    leastSquares2Grad.C

\*---------------------------------------------------------------------------*/

#ifndef leastSquares2Grad_H
#define leastSquares2Grad_H

#include "gradScheme.H"
#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

/*---------------------------------------------------------------------------*\
                      Class leastSquares2Grad Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class leastSquares2Grad
:
    public fv::gradScheme<Type>
{
    // Private Member Functions

        //- Disallow default bitwise copy construct
        leastSquares2Grad(const leastSquares2Grad&);

        //- Disallow default bitwise assignment
        void operator=(const leastSquares2Grad&);


public:

    //- Runtime type information
    TypeName("leastSquares2");


    // Constructors

        //- Construct from mesh
        leastSquares2Grad(const fvMesh& mesh)
        :
            gradScheme<Type>(mesh)
        {}

        //- Construct from Istream
        leastSquares2Grad(const fvMesh& mesh, Istream&)
        :
            gradScheme<Type>(mesh)
        {}


    // Member Functions

#if !(defined(OF16ext) && !defined(Fx40))
	virtual
#endif
        tmp
        <
            GeometricField
            <typename outerProduct<vector, Type>::type, fvPatchField, volMesh>
        > 
#if !(defined(OF16ext) && !defined(Fx40))
	calcGrad
#else
	grad
#endif
        (
            const GeometricField<Type, fvPatchField, volMesh>&
#if !(defined(OF16ext) && !defined(Fx40))
	    , const word& name
#endif
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "leastSquares2Grad.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
