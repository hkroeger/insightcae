
#include "flatPlateBLMappedFixedValueFvPatchField.H"
#include "mappedPatchBase.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //


flatPlateBLMappedFixedValueFvPatchField::flatPlateBLMappedFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(p, iF),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this)
{}



flatPlateBLMappedFixedValueFvPatchField::flatPlateBLMappedFixedValueFvPatchField
(
    const flatPlateBLMappedFixedValueFvPatchField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchField<vector>(ptf, p, iF, mapper),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this, ptf)
{}



flatPlateBLMappedFixedValueFvPatchField::flatPlateBLMappedFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<vector>(p, iF, dict),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this, dict)
{}


flatPlateBLMappedFixedValueFvPatchField::flatPlateBLMappedFixedValueFvPatchField
(
    const flatPlateBLMappedFixedValueFvPatchField& ptf
)
:
    fixedValueFvPatchField<vector>(ptf),
    mappedPatchFieldBase<vector>(ptf)
{}



flatPlateBLMappedFixedValueFvPatchField::flatPlateBLMappedFixedValueFvPatchField
(
    const flatPlateBLMappedFixedValueFvPatchField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(ptf, iF),
    mappedPatchFieldBase<vector>(this->mapper(this->patch(), iF), *this, ptf)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


const mappedPatchBase& flatPlateBLMappedFixedValueFvPatchField::mapper
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
{
    if (!isA<mappedPatchBase>(p.patch()))
    {
        FatalErrorIn
        (
            "flatPlateBLMappedFixedValueFvPatchField::mapper()"
        )   << "\n    patch type '" << p.patch().type()
            << "' not type '" << mappedPatchBase::typeName << "'"
            << "\n    for patch " << p.patch().name()
            << " of field " << iF.name()
            << " in file " << iF.objectPath()
            << exit(FatalError);
    }
    return refCast<const mappedPatchBase>(p.patch());
}



void flatPlateBLMappedFixedValueFvPatchField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }

    this->operator==(this->mappedField());

    if (debug)
    {
        Info<< "mapped on field:"
            << this->dimensionedInternalField().name()
            << " patch:" << this->patch().name()
            << "  avg:" << gAverage(*this)
            << "  min:" << gMin(*this)
            << "  max:" << gMax(*this)
            << endl;
    }

    fixedValueFvPatchField<vector>::updateCoeffs();
}


void flatPlateBLMappedFixedValueFvPatchField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    mappedPatchFieldBase<vector>::write(os);
    this->writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
