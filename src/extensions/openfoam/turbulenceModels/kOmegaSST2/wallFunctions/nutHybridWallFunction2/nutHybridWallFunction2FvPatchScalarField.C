/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright held by original author
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM; if not, write to the Free Software Foundation,
    Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

\*---------------------------------------------------------------------------*/

#include "nutHybridWallFunction2FvPatchScalarField.H"
#include "kOmegaSST2.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "addToRunTimeSelectionTable.H"
#include "wallFvPatch.H"
#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
#if OF_VERSION<030000 //not (defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806))
namespace incompressible
{
#endif
namespace RASModels
{

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void nutHybridWallFunction2FvPatchScalarField::checkType()
{
    if (!isA<wallFvPatch>(patch())) //(!patch().isWall())
    {
        FatalErrorIn("nutHybridWallFunction2FvPatchScalarField::checkType()")
            << "Invalid wall function specification" << nl
            << "    Patch type for patch " << patch().name()
            << " must be wall" << nl
            << "    Current patch type is " << patch().type() << nl << endl
            << abort(FatalError);
    }
}


scalar nutHybridWallFunction2FvPatchScalarField::calcYPlusLam
(
    const scalar kappa,
    const scalar E
) const
{
    scalar ypl = 11.0;

    for (int i=0; i<10; i++)
    {
        ypl = log(E*ypl)/kappa;
    }

    return ypl;
}


tmp<scalarField> nutHybridWallFunction2FvPatchScalarField::calcNut() const
{
    const label patchI = patch().index();

    const kOmegaSST2& rasModel 
      = db().lookupObject<kOmegaSST2>(
#if OF_VERSION>=030000 //defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806)
	"turbulenceProperties"
#else
	"RASProperties"
#endif
      );

//     const scalarField& y = rasModel.y()[patchI];
#if defined(OF_FORK_extend) //def OF16ext
    const scalarField& nuw = rasModel.nu().boundaryField()[patchI];
#else
    const scalarField& nuw = rasModel.nu()().boundaryField()[patchI];
#endif

    const scalarField& yPlusw =
      rasModel.yPlus().boundaryField()[patchI];

    const scalarField& Utauw =
      rasModel.Utau().boundaryField()[patchI];

    tmp<scalarField> tnutw(new scalarField(patch().size(), 0.0));
    scalarField& nutw = UNIOF_TMP_NONCONST(tnutw);

    const fvPatchVectorField& Uw =
      patch().lookupPatchField<volVectorField, vector>("U"); //UName_);
    
    const volVectorField& Uvol =
      db().lookupObject<volVectorField>("U"); //UName_);

    forAll(nutw, faceI)
    {
      /*
        label faceCellI = patch().faceCells()[faceI];

        scalar yPlus = Cmu25*y[faceI]*sqrt(k[faceCellI])/nuw[faceI];

        if (yPlus > yPlusLam_)
        {
            nutw[faceI] = nuw[faceI]*(yPlus*kappa_/log(E_*yPlus) - 1.0);
        }
      */
      label faceCelli = patch().faceCells()[faceI];

      scalar U = mag(Uvol[faceCelli]-Uw[faceI]);

      nutw[faceI] =
	nuw[faceI]
	*(yPlusw[faceI]*(Utauw[faceI]/(SMALL+U)) - 1.0);

    }

    return tnutw;
}


void nutHybridWallFunction2FvPatchScalarField::writeLocalEntries(Ostream& os) const
{
    os.writeKeyword("Cmu") << Cmu_ << token::END_STATEMENT << nl;
    os.writeKeyword("kappa") << kappa_ << token::END_STATEMENT << nl;
    os.writeKeyword("E") << E_ << token::END_STATEMENT << nl;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

nutHybridWallFunction2FvPatchScalarField::nutHybridWallFunction2FvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(p, iF),
    Cmu_(0.09),
    kappa_(0.41),
    E_(9.8),
    yPlusLam_(calcYPlusLam(kappa_, E_))
{
    checkType();
}


nutHybridWallFunction2FvPatchScalarField::nutHybridWallFunction2FvPatchScalarField
(
    const nutHybridWallFunction2FvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchScalarField(ptf, p, iF, mapper),
    Cmu_(ptf.Cmu_),
    kappa_(ptf.kappa_),
    E_(ptf.E_),
    yPlusLam_(ptf.yPlusLam_)
{
    checkType();
}


nutHybridWallFunction2FvPatchScalarField::nutHybridWallFunction2FvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchScalarField(p, iF, dict),
    Cmu_(dict.lookupOrDefault<scalar>("Cmu", 0.09)),
    kappa_(dict.lookupOrDefault<scalar>("kappa", 0.41)),
    E_(dict.lookupOrDefault<scalar>("E", 9.8)),
    yPlusLam_(calcYPlusLam(kappa_, E_))
{
    checkType();
}


nutHybridWallFunction2FvPatchScalarField::nutHybridWallFunction2FvPatchScalarField
(
    const nutHybridWallFunction2FvPatchScalarField& wfpsf
)
:
    fixedValueFvPatchScalarField(wfpsf),
    Cmu_(wfpsf.Cmu_),
    kappa_(wfpsf.kappa_),
    E_(wfpsf.E_),
    yPlusLam_(wfpsf.yPlusLam_)
{
    checkType();
}


nutHybridWallFunction2FvPatchScalarField::nutHybridWallFunction2FvPatchScalarField
(
    const nutHybridWallFunction2FvPatchScalarField& wfpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(wfpsf, iF),
    Cmu_(wfpsf.Cmu_),
    kappa_(wfpsf.kappa_),
    E_(wfpsf.E_),
    yPlusLam_(wfpsf.yPlusLam_)
{
    checkType();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void nutHybridWallFunction2FvPatchScalarField::updateCoeffs()
{
    operator==(calcNut());

    fixedValueFvPatchScalarField::updateCoeffs();
}


tmp<scalarField> nutHybridWallFunction2FvPatchScalarField::yPlus() const
{
  fvPatchScalarField& yPlusw = const_cast<fvPatchScalarField&>
    (patch().lookupPatchField<volScalarField, scalar>("yPlus"));
  return tmp<scalarField>(new scalarField(yPlusw));
}


void nutHybridWallFunction2FvPatchScalarField::write(Ostream& os) const
{
    fvPatchField<scalar>::write(os);
    writeLocalEntries(os);
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makePatchTypeField(fvPatchScalarField, nutHybridWallFunction2FvPatchScalarField);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
#if OF_VERSION<030000 //not (defined(OF301)||defined(OFplus)||defined(OFdev)||defined(OFesi1806))
} // End namespace incompressible
#endif
} // End namespace Foam

// ************************************************************************* //
