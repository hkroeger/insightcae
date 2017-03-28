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
    fixedMeanValueFvPatchField

Description

SourceFiles
    fixedMeanValueFvPatchField.C

\*---------------------------------------------------------------------------*/

#ifndef fixedMeanValueFvPatchField_H
#define fixedMeanValueFvPatchField_H

#include "fixedValueFvPatchFields.H"
#ifdef OFplus
#include "Function1.H"
#define DATAENTRY Function1
#else
#include "DataEntry.H"
#define DATAENTRY DataEntry
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
               Class fixedMeanValueFvPatchField Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class fixedMeanValueFvPatchField
:
    public fixedValueFvPatchField<Type>
{
    // Private data

        //- Mean value
//        Type meanValue_;
        autoPtr< DATAENTRY<Type> > meanValue_;

        //- Current time index
        label curTimeIndex_;


public:

    //- Runtime type information
    TypeName("fixedMeanValue");


    // Constructors

        //- Construct from patch and internal field
        fixedMeanValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        fixedMeanValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given fixedMeanValueFvPatchField
        //  onto a new patch
        fixedMeanValueFvPatchField
        (
            const fixedMeanValueFvPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchField<Type> > clone() const
        {
            return tmp<fvPatchField<Type> >
            (
                new fixedMeanValueFvPatchField<Type>(*this)
            );
        }

        //- Construct as copy setting internal field reference
        fixedMeanValueFvPatchField
        (
            const fixedMeanValueFvPatchField<Type>&,
            const DimensionedField<Type, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchField<Type> > clone
        (
            const DimensionedField<Type, volMesh>& iF
        ) const
        {
            return tmp<fvPatchField<Type> >
            (
                new fixedMeanValueFvPatchField<Type>(*this, iF)
            );
        }


    // Member functions

        // Access

            //- Return the ref value
            const autoPtr<DATAENTRY<Type> >& meanValue() const
            {
                return meanValue_;
            }

            //- Return reference to the ref value to allow adjustment
            autoPtr<DATAENTRY<Type> >& meanValue()
            {
                return meanValue_;
            }


        // Mapping functions

            //- Map (and resize as needed) from self given a mapping object
            virtual void autoMap
            (
                const fvPatchFieldMapper&
            );

            //- Reverse map the given fvPatchField onto this fvPatchField
            virtual void rmap
            (
                const fvPatchField<Type>&,
                const labelList&
            );


        // Evaluation functions

            //- Update the coefficients associated with the patch field
            virtual void updateCoeffs();


        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
#   include "fixedMeanValueFvPatchField.C"
#endif

#undef DATAENTRY

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
