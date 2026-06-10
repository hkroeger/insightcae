/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "limitEnergyField.H"
#include "fvMesh.H"
#include "fvMatrices.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(limitEnergyField, 0);
    addToRunTimeSelectionTable(option, limitEnergyField, dictionary);
}
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

Foam::word Foam::fv::limitEnergyField::thermoName() const
{
    if (phaseName_.empty())
    {
        return basicThermo::dictName;
    }
    return basicThermo::dictName + "." + phaseName_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::limitEnergyField::limitEnergyField
(
    const word& name,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    fv::cellSetOption(name, modelType, dict, mesh),
    phaseName_(coeffs_.getOrDefault<word>("phase", word::null)),
    Tmin_(coeffs_.get<scalar>("Tmin"))
{
    const auto& thermo = mesh_.lookupObject<basicThermo>(thermoName());

    fieldNames_.resize(1, thermo.he().name());
    fv::option::resetApplied();

    Info<< "    limitEnergyField: constraining " << thermo.he().name()
        << " to T >= " << Tmin_ << " K" << endl;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fv::limitEnergyField::constrain
(
    fvMatrix<scalar>& eqn,
    const label
)
{
    const auto& thermo = mesh_.lookupObject<basicThermo>(thermoName());

    const scalarField& TCells = thermo.T().primitiveField();
    const scalarField& pCells = thermo.p().primitiveField();

    // Collect cells below Tmin
    labelList coldCells(TCells.size());
    label nCold = 0;

    forAll(TCells, celli)
    {
        if (TCells[celli] < Tmin_)
        {
            coldCells[nCold++] = celli;
        }
    }
    coldCells.resize(nCold);

    if (nCold > 0)
    {
        scalarField pCold(pCells, coldCells);
        scalarField Tset(nCold, Tmin_);
        eqn.setValues(coldCells, thermo.he(pCold, Tset, coldCells));
    }
}


bool Foam::fv::limitEnergyField::read(const dictionary& dict)
{
    if (fv::cellSetOption::read(dict))
    {
        coeffs_.readEntry("Tmin", Tmin_);
        return true;
    }

    return false;
}


// ************************************************************************* //
