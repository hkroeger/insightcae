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

#include "extendedFixedValueFvPatchField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type>
extendedFixedValueFvPatchField<Type>::extendedFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF
)
:
    fixedValueFvPatchField<Type>(p, iF),
    vp_()
{}


template<class Type>
extendedFixedValueFvPatchField<Type>::extendedFixedValueFvPatchField
(
    const extendedFixedValueFvPatchField<Type>& ptf,
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const fvPatchFieldMapper&
)
:
    fixedValueFvPatchField<Type>(p, iF),
    vp_(ptf.vp_().clone())
{}


template<class Type>
extendedFixedValueFvPatchField<Type>::extendedFixedValueFvPatchField
(
    const fvPatch& p,
    const DimensionedField<Type, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchField<Type>(p, iF),
    vp_(FieldDataProvider<Type>::New(dict.lookup("source")))
{
  if (dict.found("value"))
  {
      fvPatchField<Type>::operator==(Field<Type>("value", dict, p.size()));
  }
  else
  {
      // Note: we use evaluate() here to trigger updateCoeffs followed
      //       by re-setting of fvatchfield::updated_ flag. This is
      //       so if first use is in the next time step it retriggers
      //       a new update.
      this->evaluate(Pstream::blocking);
  }
}


template<class Type>
extendedFixedValueFvPatchField<Type>::extendedFixedValueFvPatchField
(
    const extendedFixedValueFvPatchField<Type>& ptf
)
:
    fixedValueFvPatchField<Type>(ptf),
    vp_(ptf.vp_().clone())
{}


template<class Type>
extendedFixedValueFvPatchField<Type>::extendedFixedValueFvPatchField
(
    const extendedFixedValueFvPatchField<Type>& ptf,
    const DimensionedField<Type, volMesh>& iF
)
:
    fixedValueFvPatchField<Type>(ptf, iF),
    vp_(ptf.vp_().clone())
{
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type>
void extendedFixedValueFvPatchField<Type>::autoMap
(
    const fvPatchFieldMapper& m
)
{
    this->setSize(m.size());
}

template<class Type>
void extendedFixedValueFvPatchField<Type>::updateCoeffs()
{
    if (this->updated())
    {
        return;
    }
    
    fvPatchField<Type>::operator==( vp_()(this->db().time().timeOutputValue(), this->patch().Cf()) );    
    
    fixedValueFvPatchField<Type>::updateCoeffs();
}



template<class Type>
void extendedFixedValueFvPatchField<Type>::write(Ostream& os) const
{
    fvPatchField<Type>::write(os);
    vp_().writeEntry("source", os);
    this->writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
