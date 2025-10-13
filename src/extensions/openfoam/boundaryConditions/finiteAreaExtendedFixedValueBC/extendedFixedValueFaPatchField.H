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

#ifndef extendedFixedValueFaPatchField_H
#define extendedFixedValueFaPatchField_H

#include "faPatchFields.H"
#include "fixedValueFaPatchFields.H"

#include "fielddataprovider.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class extendedFixedValueFaPatch Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class extendedFixedValueFaPatchField
:
    public fixedValueFaPatchField<Type>
{
    // Private data

    autoPtr<FieldDataProvider<Type> > vp_;

    // struct RescaleToMassFlow {
    //     scalar massFlow;
    //     word densityFieldName;
    // };

    // struct RescaleToVolumeFlow {
    //     scalar volumeFlow;
    // };

    // struct RescaleToAverage {
    //     Type average;
    // };

    // boost::variant<
    //     boost::blank,
    //     RescaleToMassFlow,
    //     RescaleToVolumeFlow,
    //     RescaleToAverage
    //     >
    //     rescaleMode_;


public:

    //- Runtime type information
    TypeName("extendedFixedValue");


    // Constructors

        //- Construct from patch and internal field
        extendedFixedValueFaPatchField
        (
            const faPatch&,
            const DimensionedField<Type, areaMesh>&
        );

        //- Construct from patch, internal field and dictionary
        extendedFixedValueFaPatchField
        (
            const faPatch&,
            const DimensionedField<Type, areaMesh>&,
            const dictionary&
        );

        //- Construct by mapping given extendedFixedValueFaPatchField
        //  onto a new patch
        extendedFixedValueFaPatchField
        (
            const extendedFixedValueFaPatchField<Type>&,
            const faPatch&,
            const DimensionedField<Type, areaMesh>&,
            const faPatchFieldMapper&
        );

        //- Construct as copy
        extendedFixedValueFaPatchField
        (
            const extendedFixedValueFaPatchField<Type>&
        );

        //- Construct and return a clone
        virtual tmp<faPatchField<Type> > clone() const
        {
            return tmp<faPatchField<Type> >
            (
                new extendedFixedValueFaPatchField<Type>(*this)
            );
        }

        //- Construct as copy setting internal field reference
        extendedFixedValueFaPatchField
        (
            const extendedFixedValueFaPatchField<Type>&,
            const DimensionedField<Type, areaMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<faPatchField<Type> > clone
        (
            const DimensionedField<Type, areaMesh>& iF
        ) const
        {
            return tmp<faPatchField<Type> >
            (
                new extendedFixedValueFaPatchField<Type>(*this, iF)
            );
        }


    // Member functions

        // Mapping functions

            //- Map (and resize as needed) from self given a mapping object
            virtual void autoMap
            (
                const faPatchFieldMapper&
            );

        
            //- Reverse map the given faPatchField onto this faPatchField
            virtual void rmap
            (
                const faPatchField<Type>&,
                const labelList&
            );

        // Evaluation functions

            //- Update the coefficients associated with the patch field
            virtual void updateCoeffs();

        virtual void operator==(const faPatchField<Type>&);
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
makeTemplatePatchTypeFieldE(faPatchScalarField, type##FaPatchScalarField);     \
makeTemplatePatchTypeFieldE(faPatchVectorField, type##FaPatchVectorField);     \
makeTemplatePatchTypeFieldE                                                    \
(                                                                             \
    faPatchSymmTensorField,                                                   \
    type##FaPatchSymmTensorField                                              \
);                                                                            \
makeTemplatePatchTypeFieldE(faPatchTensorField, type##FaPatchTensorField);


#define makePatchTypeFieldTypedefsE(type)                                     \
                                                                              \
typedef type##FaPatchField<scalar> type##FaPatchScalarField;                  \
typedef type##FaPatchField<vector> type##FaPatchVectorField;                  \
typedef type##FaPatchField<symmTensor> type##FaPatchSymmTensorField;          \
typedef type##FaPatchField<tensor> type##FaPatchTensorField;

/*typedef type##FaPatchField<sphericalTensor>                                   \
    type##FaPatchSphericalTensorField;*/
#ifdef NoRepository
#   include "extendedFixedValueFaPatchField.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
