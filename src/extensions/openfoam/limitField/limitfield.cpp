
#include "limitfield.h"

#include "volFields.H"
#include "addToRunTimeSelectionTable.H"




// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
Foam::fv::limitField<Type>::limitField
(
    const word& name,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    cellSetOption(name, modelType, dict, mesh),
    fieldName_(coeffs_.lookup("fieldName")),
    min_(coeffs_.lookupOrDefault<scalar>("min", -GREAT)),
    max_(readScalar(coeffs_.lookup("max")))
{
    fieldNames_.setSize(1, fieldName_);
    applied_.setSize(1, false);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
bool Foam::fv::limitField<Type>::read(const dictionary& dict)
{
    if (cellSetOption::read(dict))
    {
        min_ = coeffs_.lookupOrDefault<scalar>("min", -GREAT);
        coeffs_.lookup("max") >> max_;

        return true;
    }
    else
    {
        return false;
    }
}



template<>
void Foam::fv::limitField<scalar>::correct(volScalarField& s)
{
    scalarField& sif = s.primitiveFieldRef();

    forAll(cells_, i)
    {
        const label celli = cells_[i];

        sif[celli] = min(max_, max(min_, sif[celli]));
    }

    // handle boundaries in the case of 'all'
    if (selectionMode_ == smAll)
    {
        volScalarField::Boundary& sbf = s.boundaryFieldRef();

        forAll(sbf, patchi)
        {
            fvPatchScalarField& sp = sbf[patchi];

            if (!sp.fixesValue())
            {
                forAll(sp, facei)
                {
                    sp[facei] = min(max_, max(min_, sp[facei]));
                }
            }
        }
    }
}


template<class Type>
void Foam::fv::limitField<Type>::correct(GeometricField<Type, fvPatchField, volMesh>& U)
{
    const scalar maxSqrU = sqr(max_);

    Field<Type>& Uif = U.primitiveFieldRef();

    forAll(cells_, i)
    {
        const label celli = cells_[i];

        const scalar magSqrUi = magSqr(Uif[celli]);

        if (magSqrUi > maxSqrU)
        {
            Uif[celli] *= sqrt(maxSqrU/magSqrUi);
        }
    }

    // handle boundaries in the case of 'all'
    if (selectionMode_ == smAll)
    {
        typename GeometricField<Type, fvPatchField, volMesh>::Boundary& Ubf = U.boundaryFieldRef();

        forAll(Ubf, patchi)
        {
            fvPatchField<Type>& Up = Ubf[patchi];

            if (!Up.fixesValue())
            {
                forAll(Up, facei)
                {
                    const scalar magSqrUi = magSqr(Up[facei]);

                    if (magSqrUi > maxSqrU)
                    {
                        Up[facei] *= sqrt(maxSqrU/magSqrUi);
                    }
                }
            }
        }
    }
}
