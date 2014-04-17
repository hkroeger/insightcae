/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
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
    Foam::fv::limitedSnGrad

Description
    Run-time selected snGrad scheme with limited non-orthogonal correction.

    The limiter is controlled by a coefficient with a value between 0 and 1
    which when 0 switches the correction off and the scheme behaves as
    uncorrectedSnGrad, when set to 1 the full correction of the selected scheme
    is used and when set to 0.5 the limiter is calculated such that the
    non-orthogonal contribution does not exceed the orthogonal part.

    Format:
        limited <corrected scheme> <coefficient>;

        or

        limited <coefficient>;  // Backward compatibility

SourceFiles
    limitedSnGrad.C

\*---------------------------------------------------------------------------*/

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
            return true;
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
