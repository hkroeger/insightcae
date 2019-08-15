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

#ifndef localLimitedSnGrad_H
#define localLimitedSnGrad_H

#include "correctedSnGrad.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fv
{

/*---------------------------------------------------------------------------*\
                 Class limitedSnGrad Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class localLimitedSnGrad
:
    public snGradScheme<Type>
{
    // Private data

	word limitCoeffFieldName_;

	tmp<snGradScheme<Type> > correctedScheme_;

        scalar limitCoeffMult_;


    // Private Member Functions

        //- Disallow default bitwise assignment
        void operator=(const localLimitedSnGrad&);

        //- Lookup function for the corrected to support backward compatibility
        //  of dictionary specification
        tmp<snGradScheme<Type> > lookupCorrectedScheme(Istream& schemeData)
        {
	    limitCoeffFieldName_=token(schemeData).wordToken();
	  
            token nextToken(schemeData);

            if (nextToken.isNumber())
            {
                limitCoeffMult_ = nextToken.number();
                return tmp<snGradScheme<Type> >
                (
                    new correctedSnGrad<Type>(this->mesh())
                );
            }
            else
            {
                schemeData.putBack(nextToken);
                tmp<snGradScheme<Type> > tcorrectedScheme
                (
                    fv::snGradScheme<Type>::New(this->mesh(), schemeData)
                );

                schemeData >> limitCoeffMult_;

                return tcorrectedScheme;
            }
        }


public:

    //- Runtime type information
    TypeName("localLimited");


    // Constructors

        //- Construct from mesh
        localLimitedSnGrad(const fvMesh& mesh)
        :
            snGradScheme<Type>(mesh),
            limitCoeffFieldName_(""),
            correctedScheme_(new correctedSnGrad<Type>(this->mesh())),
            limitCoeffMult_(1)
        {}


        //- Construct from mesh and data stream
        localLimitedSnGrad(const fvMesh& mesh, Istream& schemeData)
        :
            snGradScheme<Type>(mesh),
            correctedScheme_(lookupCorrectedScheme(schemeData))
        {
            if (limitCoeffMult_ < 0 || limitCoeffMult_ > 1)
            {
                FatalIOErrorIn
                (
                    "localLimitedSnGrad(const fvMesh& mesh, Istream& schemeData) : ",
                    schemeData
                )   << "limitCoeffMult is specified as " << limitCoeffMult_
                    << " but should be >= 0 && <= 1"
                    << exit(FatalIOError);
            }
        }


    //- Destructor
    virtual ~localLimitedSnGrad();


    // Member Functions

        //- Return the interpolation weighting factors for the given field
        virtual tmp<surfaceScalarField> deltaCoeffs
        (
            const GeometricField<Type, fvPatchField, volMesh>&
        ) const
        {
#ifdef OF16ext
            return this->mesh().deltaCoeffs();
#else
            return this->mesh().nonOrthDeltaCoeffs();
#endif
        }

        //- Return true if this scheme uses an explicit correction
        virtual bool corrected() const
        {
#ifdef OF16ext
	    return !this->mesh().orthogonal();
#else
            return true;
#endif
        }

        //- Return the explicit correction to the limitedSnGrad
        //  for the given field
        virtual tmp<GeometricField<Type, fvsPatchField, surfaceMesh> >
        correction(const GeometricField<Type, fvPatchField, volMesh>&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "localLimitedSnGrad.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
