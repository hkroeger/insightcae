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

#ifndef extendedFixedValueFvPatchField_H
#define extendedFixedValueFvPatchField_H

#include "Random.H"
#include "fixedValueFvPatchFields.H"

#include "fielddataprovider.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class extendedFixedValueFvPatch Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class extendedFixedValueFvPatchField
:
    public fixedValueFvPatchField<Type>
{
    // Private data

    autoPtr<FieldDataProvider<Type> > vp_;

    struct RescaleToMassFlow {
        scalar massFlow;
        word densityFieldName;
    };

    struct RescaleToVolumeFlow {
        scalar volumeFlow;
    };

    struct RescaleToAverage {
        Type average;
    };

    boost::variant<
        boost::blank,
        RescaleToMassFlow,
        RescaleToVolumeFlow,
        RescaleToAverage
        >
        rescaleMode_;


public:

    //- Runtime type information
    TypeName("extendedFixedValue");


    // Constructors

        //- Construct from patch and internal field
        extendedFixedValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        extendedFixedValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given extendedFixedValueFvPatchField
        //  onto a new patch
        extendedFixedValueFvPatchField
        (
            const extendedFixedValueFvPatchField<Type>&,
            const fvPatch&,
            const DimensionedField<Type, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        extendedFixedValueFvPatchField
        (
            const extendedFixedValueFvPatchField<Type>&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchField<Type> > clone() const
        {
            return tmp<fvPatchField<Type> >
            (
                new extendedFixedValueFvPatchField<Type>(*this)
            );
        }

        //- Construct as copy setting internal field reference
        extendedFixedValueFvPatchField
        (
            const extendedFixedValueFvPatchField<Type>&,
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
                new extendedFixedValueFvPatchField<Type>(*this, iF)
            );
        }


    // Member functions

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

        virtual void operator==(const fvPatchField<Type>&);
        virtual void operator==(const Field<Type>&);
        virtual void operator==(const Type&);

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define addToPatchFieldRunTimeSelectionE(PatchTypeField, typePatchTypeField)   \
    addToRunTimeSelectionTable                                                \
    (                                                                         \
        PatchTypeField,                                                       \
        typePatchTypeField,                                                   \
        patch                                                                 \
    );                                                                        \
    addToRunTimeSelectionTable                                                \
    (                                                                         \
        PatchTypeField,                                                       \
        typePatchTypeField,                                                   \
        patchMapper                                                           \
    );                                                                        \
    addToRunTimeSelectionTable                                                \
    (                                                                         \
        PatchTypeField,                                                       \
        typePatchTypeField,                                                   \
        dictionary                                                            \
    );


#define makeTemplatePatchTypeFieldE(PatchTypeField, typePatchTypeField)        \
    defineNamedTemplateTypeNameAndDebug(typePatchTypeField, 0);               \
    addToPatchFieldRunTimeSelectionE(PatchTypeField, typePatchTypeField)   


#define makePatchFieldsE(type)                                                \
                                                                              \
makeTemplatePatchTypeFieldE(fvPatchScalarField, type##FvPatchScalarField);     \
makeTemplatePatchTypeFieldE(fvPatchVectorField, type##FvPatchVectorField);     \
makeTemplatePatchTypeFieldE                                                    \
(                                                                             \
    fvPatchSymmTensorField,                                                   \
    type##FvPatchSymmTensorField                                              \
);                                                                            \
makeTemplatePatchTypeFieldE(fvPatchTensorField, type##FvPatchTensorField);


#define makePatchTypeFieldTypedefsE(type)                                     \
                                                                              \
typedef type##FvPatchField<scalar> type##FvPatchScalarField;                  \
typedef type##FvPatchField<vector> type##FvPatchVectorField;                  \
typedef type##FvPatchField<symmTensor> type##FvPatchSymmTensorField;          \
typedef type##FvPatchField<tensor> type##FvPatchTensorField;

/*typedef type##FvPatchField<sphericalTensor>                                   \
    type##FvPatchSphericalTensorField;*/                                       
#ifdef NoRepository
#   include "extendedFixedValueFvPatchField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
