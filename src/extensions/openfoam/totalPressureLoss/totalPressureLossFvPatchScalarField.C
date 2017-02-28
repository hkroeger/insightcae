/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011 OpenFOAM Foundation
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

#include "totalPressureLossFvPatchScalarField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "surfaceFields.H"

#include "IOField.H"
#include "pointToPointPlanarInterpolation.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

void Foam::totalPressureLossFvPatchScalarField::readCd()
{
    
    pointIOField samplePoints
    (
        IOobject
        (
            "points",
            this->db().time().constant(),
            "boundaryData"/this->patch().name(),
            this->db(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE,
            false
        )
    );

    const fileName samplePointsFile = samplePoints.filePath();

    // Allocate the interpolator
    pointToPointPlanarInterpolation mapper
    (
            samplePoints,
            this->patch().patch().faceCentres(),
//                 perturb_,
//                 nearestOnly
            1e-5
#ifndef OF22eng
            , false
#endif
    );

    const fileName samplePointsDir = samplePointsFile.path();
    instantList sampleTimes = Time::findTimes(samplePointsDir);

    // Reread values and interpolate
    IOField<scalar> vals
    (
        IOobject
        (
            "Cd",
            this->db().time().constant(),
            "boundaryData"/this->patch().name()/sampleTimes[0].name(),
            this->db(),
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE,
            false
        )
    );

    if (vals.size() != mapper.sourceSize())
    {
        FatalErrorIn
        (
            "timeVaryingMappedFixedValueFvPatchField<Type>::"
            "checkTable()"
        )   << "Number of values (" << vals.size()
            << ") differs from the number of points ("
            <<  mapper.sourceSize()
            << ") in file " << vals.objectPath() << exit(FatalError);
    }

    Cd_.reset( new scalarField(mapper.interpolate(vals)) );

    if (debug)
    {
        Pout<<"Cd = "<<Cd_()<<endl;
    }
}


Foam::totalPressureLossFvPatchScalarField::totalPressureLossFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(p, iF),
    UName_("U"),
    phiName_("phi"),
    rhoName_("none"),
    psiName_("none"),
    gamma_(0.0),
    p0_(p.size(), 0.0)
{}


Foam::totalPressureLossFvPatchScalarField::totalPressureLossFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    fixedValueFvPatchScalarField(p, iF),
    UName_(dict.lookupOrDefault<word>("U", "U")),
    phiName_(dict.lookupOrDefault<word>("phi", "phi")),
    rhoName_(dict.lookupOrDefault<word>("rho", "none")),
    psiName_(dict.lookupOrDefault<word>("psi", "none")),
    gamma_(readScalar(dict.lookup("gamma"))),
    p0_("p0", dict, p.size())
{
    if (dict.found("value"))
    {
        fvPatchField<scalar>::operator=
        (
            scalarField("value", dict, p.size())
        );
    }
    else
    {
        fvPatchField<scalar>::operator=(p0_);
    }
}


Foam::totalPressureLossFvPatchScalarField::totalPressureLossFvPatchScalarField
(
    const totalPressureLossFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    fixedValueFvPatchScalarField(ptf, p, iF, mapper),
    UName_(ptf.UName_),
    phiName_(ptf.phiName_),
    rhoName_(ptf.rhoName_),
    psiName_(ptf.psiName_),
    gamma_(ptf.gamma_),
    p0_(ptf.p0_, mapper)
{}


Foam::totalPressureLossFvPatchScalarField::totalPressureLossFvPatchScalarField
(
    const totalPressureLossFvPatchScalarField& tppsf
)
:
    fixedValueFvPatchScalarField(tppsf),
    UName_(tppsf.UName_),
    phiName_(tppsf.phiName_),
    rhoName_(tppsf.rhoName_),
    psiName_(tppsf.psiName_),
    gamma_(tppsf.gamma_),
    p0_(tppsf.p0_)
{}


Foam::totalPressureLossFvPatchScalarField::totalPressureLossFvPatchScalarField
(
    const totalPressureLossFvPatchScalarField& tppsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    fixedValueFvPatchScalarField(tppsf, iF),
    UName_(tppsf.UName_),
    phiName_(tppsf.phiName_),
    rhoName_(tppsf.rhoName_),
    psiName_(tppsf.psiName_),
    gamma_(tppsf.gamma_),
    p0_(tppsf.p0_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::totalPressureLossFvPatchScalarField::autoMap
(
    const fvPatchFieldMapper& m
)
{
    fixedValueFvPatchScalarField::autoMap(m);
    p0_.autoMap(m);
}


void Foam::totalPressureLossFvPatchScalarField::rmap
(
    const fvPatchScalarField& ptf,
    const labelList& addr
)
{
    fixedValueFvPatchScalarField::rmap(ptf, addr);

    const totalPressureLossFvPatchScalarField& tiptf =
        refCast<const totalPressureLossFvPatchScalarField>(ptf);

    p0_.rmap(tiptf.p0_, addr);
}


void Foam::totalPressureLossFvPatchScalarField::updateCoeffs
(
    const scalarField& p0p,
    const vectorField& Up
)
{
    if (updated())
    {
        return;
    }
    
    if (!Cd_.valid()) readCd();

    const fvsPatchField<scalar>& phip =
        patch().lookupPatchField<surfaceScalarField, scalar>(phiName_);

    if (psiName_ == "none" && rhoName_ == "none")
    {
        operator==(p0p - 0.5*(1.0 - pos(phip))*magSqr(Up)*(1.+Cd_()));
    }
    else if (rhoName_ == "none")
    {
        const fvPatchField<scalar>& psip =
            patch().lookupPatchField<volScalarField, scalar>(psiName_);

        if (gamma_ > 1.0)
        {
            scalar gM1ByG = (gamma_ - 1.0)/gamma_;

            operator==
            (
                p0p
               /pow
                (
                    (1.0 + 0.5*psip*gM1ByG*(1.0 - pos(phip))*magSqr(Up)*(1.+Cd_())),
                    1.0/gM1ByG
                )
            );
        }
        else
        {
            operator==(p0p/(1.0 + 0.5*psip*(1.0 - pos(phip))*magSqr(Up)*(1.+Cd_())));
        }
    }
    else if (psiName_ == "none")
    {
        const fvPatchField<scalar>& rho =
            patch().lookupPatchField<volScalarField, scalar>(rhoName_);

        operator==(p0p - 0.5*rho*(1.0 - pos(phip))*magSqr(Up)*(1.+Cd_()));
    }
    else
    {
        FatalErrorIn
        (
            "totalPressureLossFvPatchScalarField::updateCoeffs()"
        )   << " rho or psi set inconsistently, rho = " << rhoName_
            << ", psi = " << psiName_ << ".\n"
            << "    Set either rho or psi or neither depending on the "
               "definition of total pressure." << nl
            << "    Set the unused variable(s) to 'none'.\n"
            << "    on patch " << this->patch().name()
#ifdef OFdev
            << " of field " << this->internalField().name()
            << " in file " << this->internalField().objectPath()
#else
            << " of field " << this->dimensionedInternalField().name()
            << " in file " << this->dimensionedInternalField().objectPath()
#endif
            << exit(FatalError);
    }

    fixedValueFvPatchScalarField::updateCoeffs();
}


void Foam::totalPressureLossFvPatchScalarField::updateCoeffs()
{
    updateCoeffs
    (
        p0(),
        patch().lookupPatchField<volVectorField, vector>(UName())
    );
}


void Foam::totalPressureLossFvPatchScalarField::write(Ostream& os) const
{
    fvPatchScalarField::write(os);
    writeEntryIfDifferent<word>(os, "U", "U", UName_);
    writeEntryIfDifferent<word>(os, "phi", "phi", phiName_);
    os.writeKeyword("rho") << rhoName_ << token::END_STATEMENT << nl;
    os.writeKeyword("psi") << psiName_ << token::END_STATEMENT << nl;
    os.writeKeyword("gamma") << gamma_ << token::END_STATEMENT << nl;
    p0_.writeEntry("p0", os);
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        totalPressureLossFvPatchScalarField
    );
}

// ************************************************************************* //
