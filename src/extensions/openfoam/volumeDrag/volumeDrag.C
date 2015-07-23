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

#include "volumeDrag.H"
#include "fvMatrices.H"
#include "DimensionedField.H"
#include "IFstream.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(volumeDrag, 0);

    addToRunTimeSelectionTable
    (
        option,
        volumeDrag,
        dictionary
    );
}
}


// // * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //
// 
// void Foam::fv::volumeDrag::writeProps
// (
//     const scalar gradP
// ) const
// {
//     // Only write on output time
//     if (mesh_.time().outputTime())
//     {
//         IOdictionary propsDict
//         (
//             IOobject
//             (
//                 name_ + "Properties",
//                 mesh_.time().timeName(),
//                 "uniform",
//                 mesh_,
//                 IOobject::NO_READ,
//                 IOobject::NO_WRITE
//             )
//         );
//         propsDict.add("gradient", gradP);
//         propsDict.regIOobject::write();
//     }
// }

Foam::scalar Foam::fv::volumeDrag::computeAxialLength() const
{
  scalar mCD=mag(CD_);
  if (mCD<SMALL)
  {
    FatalErrorIn("Foam::fv::volumeDrag::computeAxialLength() const")
    <<"CD vector has zero length!"<<endl
    <<abort(FatalError);
  }
  
  vector el=CD_/mCD;
  scalar l = el & boundBox(UIndirectList<vector>(mesh_.C(), cells_)()).span();
  
//   Info<<"axial length l="<<l<<endl;

  return l;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::volumeDrag::volumeDrag
(
    const word& sourceName,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    option(sourceName, modelType, dict, mesh),
    CD_(coeffs_.lookup("CD"))
{
    fieldNames_.setSize(1);
    fieldNames_[0]="U";
    applied_.setSize(fieldNames_.size(), false);

    Info<< "    Setting volume drag CD = " << CD_ << nl << endl;
}

bool Foam::fv::volumeDrag::read(const Foam::dictionary& dict)
{
  if (option::read(dict))
  {
    coeffs_.lookup("CD") >> CD_;
    
    return true;
  }
  else
  {
    return false;
  }
}



// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::fv::volumeDrag::correct(volVectorField& U)
{
//     const scalarField& rAU = invAPtr_().internalField();
// 
//     // Integrate flow variables over cell set
//     scalar magUbarAve = 0.0;
//     scalar rAUave = 0.0;
//     const scalarField& cv = mesh_.V();
//     forAll(cells_, i)
//     {
//         label cellI = cells_[i];
//         scalar volCell = cv[cellI];
//         magUbarAve += (flowDir_ & U[cellI])*volCell;
//         rAUave += rAU[cellI]*volCell;
//     }
// 
//     // Collect across all processors
//     reduce(magUbarAve, sumOp<scalar>());
//     reduce(rAUave, sumOp<scalar>());
// 
//     // Volume averages
//     magUbarAve /= V_;
//     rAUave /= V_;
// 
//     // Calculate the pressure gradient increment needed to adjust the average
//     // flow-rate to the desired value
//     dGradP_ = (mag(Ubar_) - magUbarAve)/rAUave;
// 
//     // Apply correction to velocity field
//     forAll(cells_, i)
//     {
//         label cellI = cells_[i];
//         U[cellI] += flowDir_*rAU[cellI]*dGradP_;
//     }
// 
//     scalar gradP = gradP0_ + dGradP_;
// 
//     Info<< "Pressure gradient source: uncorrected Ubar = " << magUbarAve
//         << ", pressure gradient = " << gradP << endl;
// 
//     writeProps(gradP);
}


Foam::tmp<Foam::vectorField> Foam::fv::volumeDrag::computeSup(fvMatrix<vector>& eqn)
{
    tmp<vectorField> Su(new vectorField(cells_.size(), vector::zero));
    
    const vectorField& U = eqn.psi();
    
    Su() = -0.5*CD_* magSqr(UIndirectList<vector>(U, cells_)()) / computeAxialLength();

    return Su;
}


void Foam::fv::volumeDrag::addSup
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

    
    UIndirectList<vector>(Su, cells_) = computeSup(eqn);
    
//     if (mesh_.time().outputTime()) Su.write();

    eqn += Su;
}


void Foam::fv::volumeDrag::addSup
(
    const volScalarField& rho,
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

    
    UIndirectList<vector>(Su, cells_) = rho*computeSup(eqn);

    eqn += Su;
}


// ************************************************************************* //
