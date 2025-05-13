/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
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

#include "pressureOutletUniformVelocityFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::pressureOutletUniformVelocityFvPatchVectorField::
pressureOutletUniformVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    pressureInletVelocityFvPatchVectorField(p, iF)
{}


Foam::pressureOutletUniformVelocityFvPatchVectorField::
pressureOutletUniformVelocityFvPatchVectorField
(
    const pressureOutletUniformVelocityFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    pressureInletVelocityFvPatchVectorField(ptf, p, iF, mapper)
{}


Foam::pressureOutletUniformVelocityFvPatchVectorField::
pressureOutletUniformVelocityFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    pressureInletVelocityFvPatchVectorField(p, iF, dict)
{}


Foam::pressureOutletUniformVelocityFvPatchVectorField::
pressureOutletUniformVelocityFvPatchVectorField
(
    const pressureOutletUniformVelocityFvPatchVectorField& pivpvf
)
:
    pressureInletVelocityFvPatchVectorField(pivpvf)
{}


Foam::pressureOutletUniformVelocityFvPatchVectorField::
pressureOutletUniformVelocityFvPatchVectorField
(
    const pressureOutletUniformVelocityFvPatchVectorField& pivpvf,
    const DimensionedField<vector, volMesh>& iF
)
:
    pressureInletVelocityFvPatchVectorField(pivpvf, iF)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::pressureOutletUniformVelocityFvPatchVectorField::updateCoeffs()
{
    if (updated())
    {
        return;
    }

    pressureInletVelocityFvPatchVectorField::updateCoeffs();

    // Get the tangential component from the internalField (zero-gradient)
    tmp<vectorField> n = patch().nf();
    vectorField Ut(patchInternalField());
    Ut -= n()*(Ut & n());

    operator==(Ut+patch().nf()*gSum(patch().Sf() & *this)/gSum(patch().magSf()));
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void Foam::pressureOutletUniformVelocityFvPatchVectorField::operator=
(
    const fvPatchField<vector>& pvf
)
{
    // Get the tangential component from the internalField (zero-gradient)
    tmp<vectorField> n = patch().nf();
    vectorField Ut(patchInternalField());
    Ut -= n()*(Ut & n());

    operator==(Ut+patch().nf()*gSum(patch().Sf() & pvf)/gSum(patch().magSf()));
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        pressureOutletUniformVelocityFvPatchVectorField
    );
}

// ************************************************************************* //
