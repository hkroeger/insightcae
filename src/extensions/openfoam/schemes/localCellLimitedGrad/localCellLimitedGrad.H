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

#ifndef localCellLimitedGrad_H
#define localCellLimitedGrad_H

#include "gradScheme.H"
#include "surfaceFields.H"
#include "fvcAverage.H"
#include "zeroGradientFvPatchField.H"
#include "uniof.h"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

  
template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh> >
surfaceMax
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf
)
{
    const fvMesh& mesh = ssf.mesh();
    
    tmp<GeometricField<Type, fvPatchField, volMesh> > tvf
    (
        new GeometricField<Type, fvPatchField, volMesh>
        (
            IOobject
            (
                "surfaceMax("+ssf.name()+')',
                ssf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<Type>
            (
                "0",
                ssf.dimensions(),
                pTraits<Type>::zero
            ),
            zeroGradientFvPatchField<Type>::typeName
        )
    );
    
    GeometricField<Type, fvPatchField, volMesh>& vf = UNIOF_TMP_NONCONST(tvf);

    const UNIOF_LABELULIST& owner = mesh.owner();
    const UNIOF_LABELULIST& neighbour = mesh.neighbour();

    const Field<Type>& issf = ssf;

    forAll(owner, facei)
    {
        vf[owner[facei]] = max(vf[owner[facei]], issf[facei]);
        vf[neighbour[facei]] = max(vf[neighbour[facei]], issf[facei]);
    }

    forAll(mesh.boundary(), patchi)
    {
        const UNIOF_LABELULIST& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        const fvsPatchField<Type>& pssf = ssf.boundaryField()[patchi];

        forAll(mesh.boundary()[patchi], facei)
        {
            vf[pFaceCells[facei]] = max(vf[pFaceCells[facei]], pssf[facei]);
        }
    }

    return tvf;
}

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

	tmp<volScalarField> getk(const fvMesh& mesh) const
	{
	    tmp<volScalarField> k
	    (
	      new volScalarField
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
	      UNIOF_TMP_NONCONST(k) = 
	      max
	      (
              1e-3, 
              surfaceMax(mesh.lookupObject<surfaceScalarField>(kname_))
          );
	    }
	    else
	    {
	      WarningIn("localCellLimitedGrad::getk")
	      <<"Limit coefficient field "<<kname_<<" not found. Turning limiter everywhere on for this call."
	      <<endl;
	    }
	    
	    return k;
	}
	
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
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
	grad
#else
        calcGrad
#endif
        (
            const GeometricField<Type, fvPatchField, volMesh>& vsf
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
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
) const;


template<>
tmp<volTensorField> localCellLimitedGrad<vector>::
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
grad
#else
calcGrad
#endif
(
    const volVectorField& vsf
#if (defined(OF_FORK_extend) && OF_VERSION<010602) //(defined(OF16ext) && !defined(Fx32) && !defined(Fx40) && !defined(Fx41))
#else
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
