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
    Tmin_(coeffs_.getOrDefault<scalar>("Tmin", scalar(0))),
    Tmax_(coeffs_.getOrDefault<scalar>("Tmax", scalar(GREAT)))
{
    const auto& thermo = mesh_.lookupObject<basicThermo>(thermoName());

    fieldNames_.resize(1, thermo.he().name());
    fv::option::resetApplied();

    Info<< "    limitEnergyField: constraining " << thermo.he().name()
        << " to T in [" << Tmin_ << ", " << Tmax_ << "] K" << endl;
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fv::limitEnergyField::constrain
(
    fvMatrix<scalar>& eqn,
    const label
)
{
    // Fires inside eqn.solve() before the linear solve.
    // Replaces matrix rows to force he = he(p, Tbound) where T is out of range.

    const auto& thermo = mesh_.lookupObject<basicThermo>(thermoName());

    const scalarField& TCells = thermo.T().primitiveField();
    const scalarField& pCells = thermo.p().primitiveField();

    labelList boundCells(TCells.size());
    scalarField Ttarget(TCells.size());
    label nBound = 0;

    forAll(TCells, celli)
    {
        if (TCells[celli] < Tmin_)
        {
            boundCells[nBound] = celli;
            Ttarget[nBound]    = Tmin_;
            ++nBound;
        }
        else if (TCells[celli] > Tmax_)
        {
            boundCells[nBound] = celli;
            Ttarget[nBound]    = Tmax_;
            ++nBound;
        }
    }
    boundCells.resize(nBound);
    Ttarget.resize(nBound);

    if (nBound > 0)
    {
        scalarField pBound(pCells, boundCells);
        eqn.setValues(boundCells, thermo.he(pBound, Ttarget, boundCells));
    }

    Foam::reduce(nBound, sumOp<label>());

    if (nBound > 0)
    {
        Info<<"Limited "<<thermo.T().name()<<" in "<<nBound<<" cells."<<endl;
    }
}


void Foam::fv::limitEnergyField::correct(volScalarField& he)
{
    // Fires after the energy solve and correctThermo.
    //
    // correctThermo uses the stored T field as the Newton initial guess T0.
    // If correctThermo produced a negative T (from a slightly negative he
    // that slipped past the matrix constraint), the *next* correctThermo
    // call crashes on the T0 < 0 guard in thermoI.H, even if the new he
    // is properly bounded.
    //
    // This method clips both he AND T to prevent that stale negative T0.

    const auto& thermo = mesh_.lookupObject<basicThermo>(thermoName());

    scalarField& heCells = he.primitiveFieldRef();

    // T is conceptually a derived quantity (T = THE(he, p)) but the thermo
    // object caches it as a volScalarField that persists between corrector
    // calls and is used as T0 on the next call.  We must keep it consistent.
    volScalarField& T =
        const_cast<volScalarField&>(thermo.T());
    scalarField& TCells = T.primitiveFieldRef();

    // Build lists of out-of-range cells separately for min and max so we
    // can call thermo.he(p_subset, T_subset, cells_subset) which is the
    // same API used by limitTemperature::correct().

    labelList coldCells(heCells.size());
    labelList hotCells(heCells.size());
    label nCold = 0, nHot = 0;

    forAll(TCells, celli)
    {
        if (TCells[celli] < Tmin_)
        {
            coldCells[nCold++] = celli;
        }
        else if (TCells[celli] > Tmax_)
        {
            hotCells[nHot++] = celli;
        }
    }
    coldCells.resize(nCold);
    hotCells.resize(nHot);

    if (nCold > 0)
    {
        const scalarField pCold(thermo.p().primitiveField(), coldCells);
        const scalarField Tset(nCold, Tmin_);
        const scalarField heMin(thermo.he(pCold, Tset, coldCells));

        forAll(coldCells, i)
        {
            heCells[coldCells[i]] = heMin[i];
            TCells[coldCells[i]]  = Tmin_;
        }
    }

    if (nHot > 0)
    {
        const scalarField pHot(thermo.p().primitiveField(), hotCells);
        const scalarField Tset(nHot, Tmax_);
        const scalarField heMax(thermo.he(pHot, Tset, hotCells));

        forAll(hotCells, i)
        {
            heCells[hotCells[i]] = heMax[i];
            TCells[hotCells[i]]  = Tmax_;
        }
    }

    // Boundaries
    if (!cellSetOption::useSubMesh())
    {
        volScalarField::Boundary& heBf = he.boundaryFieldRef();
        volScalarField::Boundary& TBf  = T.boundaryFieldRef();

        forAll(heBf, patchi)
        {
            fvPatchScalarField& hep = heBf[patchi];
            fvPatchScalarField& Tp  = TBf[patchi];

            if (!hep.fixesValue())
            {
                const scalarField& pp =
                    thermo.p().boundaryField()[patchi];

                const scalarField heMinp
                (
                    thermo.he(pp, scalarField(pp.size(), Tmin_), patchi)
                );
                const scalarField heMaxp
                (
                    thermo.he(pp, scalarField(pp.size(), Tmax_), patchi)
                );

                forAll(hep, facei)
                {
                    if (hep[facei] < heMinp[facei])
                    {
                        hep[facei] = heMinp[facei];
                        Tp[facei]  = Tmin_;
                    }
                    else if (hep[facei] > heMaxp[facei])
                    {
                        hep[facei] = heMaxp[facei];
                        Tp[facei]  = Tmax_;
                    }
                }
            }
        }
    }
}


bool Foam::fv::limitEnergyField::read(const dictionary& dict)
{
    if (fv::cellSetOption::read(dict))
    {
        coeffs_.readIfPresent("Tmin", Tmin_);
        coeffs_.readIfPresent("Tmax", Tmax_);
        return true;
    }

    return false;
}


// ************************************************************************* //
