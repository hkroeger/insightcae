#ifndef flatPlateBLMappedFixedValueFvPatchField_H
#define flatPlateBLMappedFixedValueFvPatchField_H

#include "fixedValueFvPatchFields.H"
//#include "interpolation.H"
#include "mappedPatchFieldBase.H"
#include "pointToPointPlanarInterpolation.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                Class flatPlateBLMappedFixedValueFvPatchField Declaration
\*---------------------------------------------------------------------------*/

class flatPlateBLMappedFixedValueFvPatchField
:
    public fixedValueFvPatchField<vector>,
    public mappedPatchFieldBase<vector>
{

protected:
  
  autoPtr<pointToPointPlanarInterpolation> innermapping_;
  autoPtr<pointToPointPlanarInterpolation> outermapping_;
  vectorField Umean_recy;

    // Protected Member Functions

        const mappedPatchBase& mapper
        (
            const fvPatch& p,
            const DimensionedField<vector, volMesh>& iF
        );


public:

    //- Runtime type information
    TypeName("flatPlateBLMapped");


    // Constructors

        //- Construct from patch and internal field
        flatPlateBLMappedFixedValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        flatPlateBLMappedFixedValueFvPatchField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given flatPlateBLMappedFixedValueFvPatchField
        //  onto a new patch
        flatPlateBLMappedFixedValueFvPatchField
        (
            const flatPlateBLMappedFixedValueFvPatchField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        flatPlateBLMappedFixedValueFvPatchField
        (
            const flatPlateBLMappedFixedValueFvPatchField&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchField<vector> > clone() const
        {
            return tmp<fvPatchField<vector> >
            (
                new flatPlateBLMappedFixedValueFvPatchField(*this)
            );
        }

        //- Construct as copy setting internal field reference
        flatPlateBLMappedFixedValueFvPatchField
        (
            const flatPlateBLMappedFixedValueFvPatchField&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchField<vector> > clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchField<vector> >
            (
                new flatPlateBLMappedFixedValueFvPatchField(*this, iF)
            );
        }


    // Member functions

        // Evaluation functions

            //- Update the coefficients associated with the patch field
            virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// #ifdef NoRepository
// #   include "flatPlateBLMappedFixedValueFvPatchField.C"
// #endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
