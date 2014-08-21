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
    Foam::fv::localFaceLimitedGrad

Description
    localFaceLimitedGrad gradient scheme applied to a runTime selected base gradient
    scheme.

    The scalar limiter based on limiting the extrapolated face values
    between the face-neighbour cell values and is applied to all components
    of the gradient.

SourceFiles
    localFaceLimitedGrad.C

\*---------------------------------------------------------------------------*/

#ifndef localFaceLimitedGrad_H
#define localFaceLimitedGrad_H

#include "gradScheme.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

/*---------------------------------------------------------------------------*\
                       Class localFaceLimitedGrad Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class localFaceLimitedGrad
:
    public fv::gradScheme<Type>
{
    // Private Data

        tmp<fv::gradScheme<Type> > basicGradScheme_;

        //- Limiter coefficient
        const word kname_;


    // Private Member Functions

        inline void limitFace
        (
            scalar& limiter,
            const scalar maxDelta,
            const scalar minDelta,
            const scalar extrapolate
        ) const;


        //- Disallow default bitwise copy construct
        localFaceLimitedGrad(const localFaceLimitedGrad&);

        //- Disallow default bitwise assignment
        void operator=(const localFaceLimitedGrad&);
	
	tmp<surfaceScalarField> getk(const fvMesh& mesh) const
	{
	    tmp<surfaceScalarField> k
	    (
	      new surfaceScalarField
	      (
		  IOobject
		    (
			"gradLimiterCoeff",
			mesh.time().timeName(),
			mesh,
			IOobject::NO_READ,
			IOobject::NO_WRITE
		    ),
		    mesh,
		    dimensionedScalar("k", dimless, 1.0)  // limit everywhere
		)
	    );
	    if (mesh.foundObject<surfaceScalarField>(kname_))
	    {
	      k() = min(1.0, max(1e-3, mesh.lookupObject<surfaceScalarField>(kname_)));
	    }
	    else
	    {
	      WarningIn("localFaceLimitedGrad::getk")
	      <<"Limit coefficient field "<<kname_<<" not found. Turning limiter everywhere on for this call."
	      <<endl;
	    }
	    
	    return k;
	}


public:

    //- RunTime type information
    TypeName("localFaceLimited");


    // Constructors

        //- Construct from mesh and schemeData
        localFaceLimitedGrad(const fvMesh& mesh, Istream& schemeData)
        :
            gradScheme<Type>(mesh),
            basicGradScheme_(fv::gradScheme<Type>::New(mesh, schemeData)),
            kname_(schemeData)
        {
	  /*
            if (k_ < 0 || k_ > 1)
            {
                FatalIOErrorIn
                (
                    "localFaceLimitedGrad(const fvMesh&, Istream& schemeData)",
                    schemeData
                )   << "coefficient = " << k_
                    << " should be >= 0 and <= 1"
                    << exit(FatalIOError);
            }
            */
        }


    // Member Functions

        //- Return the gradient of the given field to the gradScheme::grad
        //  for optional caching
        virtual tmp
        <
            GeometricField
            <typename outerProduct<vector, Type>::type, fvPatchField, volMesh>
        > calcGrad
        (
            const GeometricField<Type, fvPatchField, volMesh>& vsf,
            const word& name
        ) const
        {
            return grad(vsf);
        }
};


// * * * * * * * * * * * * Inline Member Function  * * * * * * * * * * * * * //

template<class Type>
inline void localFaceLimitedGrad<Type>::limitFace
(
    scalar& limiter,
    const scalar maxDelta,
    const scalar minDelta,
    const scalar extrapolate
) const
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


// * * * * * * * * Template Member Function Specialisations  * * * * * * * * //

template<>
tmp<volVectorField> localFaceLimitedGrad<scalar>::calcGrad
(
    const volScalarField& vsf,
    const word& name
) const;


template<>
tmp<volTensorField> localFaceLimitedGrad<vector>::calcGrad
(
    const volVectorField& vsf,
    const word& name
) const;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
