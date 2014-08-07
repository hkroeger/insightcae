/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::fv::cellLimitedGrad

Description
    cellLimitedGrad gradient scheme applied to a runTime selected base gradient
    scheme.

    The scalar limiter based on limiting the extrapolated face values
    between the maximum and minumum cell and cell neighbour values and is
    applied to all components of the gradient.

SourceFiles
    cellLimitedGrad.C

\*---------------------------------------------------------------------------*/

#ifndef localCellLimitedGrad_H
#define localCellLimitedGrad_H

#include "gradScheme.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#include "../../../../../../OpenFOAM/OpenFOAM-2.1.x/src/finiteVolume/finiteVolume/gradSchemes/gradScheme/gradScheme.H"

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

/*---------------------------------------------------------------------------*\
                       Class localCellLimitedyGrad Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class localCellLimitedGrad
:
    public fv::gradScheme<Type>
{
    // Private Data

        tmp<fv::gradScheme<Type> > basicGradScheme_;

        //- Limiter coefficient field name
        const word kname_;


    // Private Member Functions

        //- Disallow default bitwise copy construct
        localCellLimitedGrad(const localCellLimitedGrad&);

        //- Disallow default bitwise assignment
        void operator=(const localCellLimitedGrad&);


public:

    //- RunTime type information
    TypeName("localCellLimited");


    // Constructors

        //- Construct from mesh and schemeData
        localCellLimitedGrad(const fvMesh& mesh, Istream& schemeData)
        :
            gradScheme<Type>(mesh),
            basicGradScheme_(fv::gradScheme<Type>::New(mesh, schemeData)),
            kname_(schemeData)
        {
//             if (k_ < 0 || k_ > 1)
//             {
//                 FatalIOErrorIn
//                 (
//                     "localCellLimitedGrad(const fvMesh&, Istream& schemeData)",
//                     schemeData
//                 )   << "coefficient = " << k_
//                     << " should be >= 0 and <= 1"
//                     << exit(FatalIOError);
//             }
        }


    // Member Functions

        static inline void limitFace
        (
            Type& limiter,
            const Type& maxDelta,
            const Type& minDelta,
            const Type& extrapolate
        );

        //- Return the gradient of the given field to the gradScheme::grad
        //  for optional caching
        virtual tmp
        <
            GeometricField
            <typename outerProduct<vector, Type>::type, fvPatchField, volMesh>
        > 
#ifdef OF16ext
	grad
#else
        calcGrad
#endif
        (
            const GeometricField<Type, fvPatchField, volMesh>& vsf
#ifdef OF16ext
#else
            ,
            const word& name
#endif
        ) const;
};


// * * * * * * * * * * * * Inline Member Function  * * * * * * * * * * * * * //

template<>
inline void localCellLimitedGrad<scalar>::limitFace
(
    scalar& limiter,
    const scalar& maxDelta,
    const scalar& minDelta,
    const scalar& extrapolate
)
{
    if (extrapolate > maxDelta + VSMALL)
    {
        limiter = min(limiter, maxDelta/extrapolate);
    }
    else if (extrapolate < minDelta - VSMALL)
    {
        limiter = min(limiter, minDelta/extrapolate);
    }
}


template<class Type>
inline void localCellLimitedGrad<Type>::limitFace
(
    Type& limiter,
    const Type& maxDelta,
    const Type& minDelta,
    const Type& extrapolate
)
{
    for (direction cmpt=0; cmpt<Type::nComponents; cmpt++)
    {
        localCellLimitedGrad<scalar>::limitFace
        (
            limiter.component(cmpt),
            maxDelta.component(cmpt),
            minDelta.component(cmpt),
            extrapolate.component(cmpt)
        );
    }
}


// * * * * * * * * Template Member Function Specialisations  * * * * * * * * //

template<>
tmp<volVectorField> localCellLimitedGrad<scalar>::
#ifdef OF16ext
grad
#else
calcGrad
#endif
(
    const volScalarField& vsf
#ifndef OF16ext
    ,
    const word& name
#endif
) const;


template<>
tmp<volTensorField> localCellLimitedGrad<vector>::
#ifdef OF16ext
grad
#else
calcGrad
#endif
(
    const volVectorField& vsf
#ifndef OF16ext
    ,
    const word& name
#endif
) const;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
