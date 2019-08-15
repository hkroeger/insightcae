/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "fvPatchField.H"
#include "addToRunTimeSelectionTable.H"
#include "mappedWithSwirlFixedValueFvPatchVectorField.H"
#include "mappedPatchBase.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

mappedWithSwirlFixedValueFvPatchVectorField::mappedWithSwirlFixedValueFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(p, iF),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this),
    p0_(point::zero),
    ax_(0,0,1),
    S_targ_(1.0)
{}


mappedWithSwirlFixedValueFvPatchVectorField::mappedWithSwirlFixedValueFvPatchVectorField
(
    const mappedWithSwirlFixedValueFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchField<vector>(ptf, p, iF, mapper),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this, ptf),
    p0_(ptf.p0_),
    ax_(ptf.ax_),
    S_targ_(ptf.S_targ_)
{}


mappedWithSwirlFixedValueFvPatchVectorField::mappedWithSwirlFixedValueFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<vector>(p, iF, dict),
    mappedPatchFieldBase<vector>(this->mapper(p, iF), *this, dict),
    p0_(dict.lookup("p0")),
    ax_(dict.lookup("ax")),
    S_targ_(readScalar(dict.lookup("S_targ")))
{}


mappedWithSwirlFixedValueFvPatchVectorField::mappedWithSwirlFixedValueFvPatchVectorField
(
    const mappedWithSwirlFixedValueFvPatchVectorField& ptf
)
:
    fixedValueFvPatchField<vector>(ptf),
    mappedPatchFieldBase<vector>(ptf),
    p0_(ptf.p0_),
    ax_(ptf.ax_),
    S_targ_(ptf.S_targ_)
{}


mappedWithSwirlFixedValueFvPatchVectorField::mappedWithSwirlFixedValueFvPatchVectorField
(
    const mappedWithSwirlFixedValueFvPatchVectorField& ptf,
    const DimensionedField<vector, volMesh>& iF
)
:
    fixedValueFvPatchField<vector>(ptf, iF),
    mappedPatchFieldBase<vector>(this->mapper(this->patch(), iF), *this, ptf),
    p0_(ptf.p0_),
    ax_(ptf.ax_),
    S_targ_(ptf.S_targ_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

const mappedPatchBase& mappedWithSwirlFixedValueFvPatchVectorField::mapper
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
{
    if (!isA<mappedPatchBase>(p.patch()))
    {
        FatalErrorIn
        (
            "mappedWithSwirlFixedValueFvPatchVectorField::mapper()"
        )   << "\n    patch type '" << p.patch().type()
            << "' not type '" << mappedPatchBase::typeName << "'"
            << "\n    for patch " << p.patch().name()
            << " of field " << iF.name()
            << " in file " << iF.objectPath()
            << exit(FatalError);
    }
    return refCast<const mappedPatchBase>(p.patch());
}


void mappedWithSwirlFixedValueFvPatchVectorField::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }
    
    vectorField er=patch().Cf()-p0_;
    scalarField r=mag(er);
    er/=SMALL+r;
    
    vectorField et=ax_^er;
    forAll(et, j)
    {
      if (r[j]<SMALL) 
	et[j]*=0.0;
    }
    
    vectorField umapped=this->mappedField();
    scalarField u=umapped&ax_;
    scalarField w=umapped&et;
    scalar S = 
      gSum(w*u*r*patch().magSf())
      /
      gSum(u*u*patch().magSf());
      
    double sw=S_targ_/S;
    
    Info<<"Swirl number S="<<S<<", rescaling factor sw="<<sw<<endl;

    this->operator==(umapped + (sw-1.0)*w*et);

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


void mappedWithSwirlFixedValueFvPatchVectorField::write(Ostream& os) const
{
    fvPatchField<vector>::write(os);
    mappedPatchFieldBase<vector>::write(os);
    os << "p0" << token::SPACE << p0_ << token::END_STATEMENT;
    os << "ax" << token::SPACE << ax_ << token::END_STATEMENT;
    os << "S_targ" << token::SPACE << S_targ_ << token::END_STATEMENT;
    this->writeEntry("value", os);
}

makePatchTypeField
(
    fvPatchVectorField,
    mappedWithSwirlFixedValueFvPatchVectorField
);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
