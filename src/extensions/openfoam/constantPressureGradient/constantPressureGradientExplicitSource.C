/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2014 OpenFOAM Foundation
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

#include "constantPressureGradientExplicitSource.H"
#include "fvMatrices.H"
#include "DimensionedField.H"
#include "IFstream.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(constantPressureGradientExplicitSource, 0);

    addToRunTimeSelectionTable
    (
        option,
        constantPressureGradientExplicitSource,
        dictionary
    );
}
}


void Foam::fv::constantPressureGradientExplicitSource::correct(volVectorField& U)
{
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::constantPressureGradientExplicitSource::constantPressureGradientExplicitSource
(
    const word& sourceName,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
#if defined(OFplus)
    cellSetOption(sourceName, modelType, dict, mesh),
#else
    option(sourceName, modelType, dict, mesh),
#endif
    gradP_(coeffs_.lookup("gradP"))
{
     coeffs_.lookup("fieldNames") >> fieldNames_;
//  fieldNames_.resize(1);
//  fieldNames_[0]="U";


    applied_.setSize(fieldNames_.size(), false);

}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


void Foam::fv::constantPressureGradientExplicitSource::addSup
(
    fvMatrix<vector>& eqn,
    const label fieldI
)
{
    DimensionedField<vector, volMesh> Su
    (
        IOobject
        (
            name_ + fieldNames_[fieldI] + "Sup",
            mesh_.time().timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh_,
        dimensionedVector("zero", eqn.dimensions()/dimVolume, vector::zero)
    );

    if (Su.dimensions()!=gradP_.dimensions())
    {
      FatalErrorIn("Foam::fv::constantPressureGradientExplicitSource::addSup")
       << "incompatible dimensions: "<<Su.dimensions()<<" <=> "<<gradP_.dimensions()
       << abort(FatalError);
    }

    UIndirectList<vector>(Su, cells_) = gradP_.value();

    eqn += Su;
}


void Foam::fv::constantPressureGradientExplicitSource::addSup
(
    const volScalarField& rho,
    fvMatrix<vector>& eqn,
    const label fieldI
)
{
    this->addSup(eqn, fieldI);
}



// ************************************************************************* //
