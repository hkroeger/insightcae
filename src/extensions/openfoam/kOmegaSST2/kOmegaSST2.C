/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 1991-2009 OpenCFD Ltd.
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

#include "fvCFD.H"
#include "kOmegaSST2.H"
#include "addToRunTimeSelectionTable.H"
#include "fixedInternalValueFvPatchFields.H"
#include "gaussConvectionScheme.H"

#if not (defined(OF301) || defined(OFplus) || defined(OFdev))
#include "backwardsCompatibilityWallFunctions.H"
#else
#include "bound.H"
#endif

#include "uniof.h"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
#if not (defined(OF301) || defined(OFplus) || defined(OFdev))
namespace incompressible
{
#endif
namespace RASModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(kOmegaSST2, 0);
#if defined(OF301) || defined(OFplus) || defined(OFdev)
addToRunTimeSelectionTable
(
    RAStransportModelIncompressibleTurbulenceModel,
    kOmegaSST2,
    dictionary
);
#else
addToRunTimeSelectionTable(RASModel, kOmegaSST2, dictionary);
#endif


// * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * * //

tmp<volScalarField> kOmegaSST2::F1(const volScalarField& CDkOmega) const
{
    volScalarField CDkOmegaPlus = max
    (
        CDkOmega,
        dimensionedScalar("1.0e-21", dimless/sqr(dimTime), 1.0e-21)
    );

    volScalarField arg1 = min
    (
        max
        (
            (scalar(1)/betaStar_)*sqrt(k_)/max(omega_*y_, omegaSmall_*ySmall_),
            scalar(500)*nu()/max(sqr(y_)*omega_, sqr(ySmall_)*omegaSmall_)
        ),
        (4*alphaOmega2_)*k_/(CDkOmegaPlus*sqr(y_))
    );


    return tanh(pow4(arg1));
}

tmp<volScalarField> kOmegaSST2::F2() const
{
    volScalarField arg2 = max
    (
        (scalar(2)/betaStar_)*sqrt(k_)/max(omega_*y_, omegaSmall_*ySmall_),
        scalar(500)*nu()/max(sqr(y_)*omega_, sqr(ySmall_)*omegaSmall_)
    );

    return tanh(sqr(arg2));
}

#ifndef OF16ext
scalar kOmegaSST2::yPlusLam(const scalar kappa, const scalar E) const
{
    scalar ypl = 11.0;

    for (int i=0; i<10; i++)
    {
        ypl = log(max(E*ypl, 1))/kappa;
    }

    return ypl;
}
#endif


void kOmegaSST2::correctNut
(
    const volScalarField& S2
)
{
    // Correct the turbulence viscosity
//     this->nut_ = a1_*k_/max(a1_*omega_, b1_*F23()*sqrt(S2));
    this->nut_ = a1_*k_/max(a1_*omega_, F2()*sqrt(2.0*S2));
    this->nut_.correctBoundaryConditions();
//     fv::options::New(this->mesh_).correct(this->nut_);
}


void kOmegaSST2::correctNut()
{
    correctNut(2*magSqr(symm(fvc::grad(this->U_))));
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

kOmegaSST2::kOmegaSST2
(
#if defined(OF301) || defined(OFplus) || defined(OFdev)
     const alphaField& alpha,
    const rhoField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const transportModel& transport,
    const word& propertiesName,
    const word& type 
#else
    const volVectorField& U,
    const surfaceScalarField& phi,
    transportModel& lamTransportModel
#ifndef OF16ext
    ,
    const word& turbulenceModelName,
    const word& modelName
#endif
#endif
)
:
#if defined(OF16ext)
    RASModel(typeName, U, phi, lamTransportModel),
#elif defined(OF301) || defined(OFplus) || defined(OFdev)
    RAStransportModelIncompressibleTurbulenceModel
    (
      type,
        alpha,
        rho,
        U,
        alphaRhoPhi,
        phi,
        transport,
        propertiesName
    ),
#else
    RASModel(modelName, U, phi, lamTransportModel, turbulenceModelName),
#endif

    alphaK1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaK1",
            coeffDict_,
            0.85034
        )
    ),
    alphaK2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaK2",
            coeffDict_,
            1.0
        )
    ),
    alphaOmega1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaOmega1",
            coeffDict_,
            0.5
        )
    ),
    alphaOmega2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "alphaOmega2",
            coeffDict_,
            0.85616
        )
    ),
    gamma1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma1",
            coeffDict_,
            0.5532
        )
    ),
    gamma2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma2",
            coeffDict_,
            0.4403
        )
    ),
    beta1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta1",
            coeffDict_,
            0.075
        )
    ),
    beta2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta2",
            coeffDict_,
            0.0828
        )
    ),
    betaStar_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "betaStar",
            coeffDict_,
            0.09
        )
    ),
    cc_
    (
      Switch::lookupOrAddToDict
      (
	  "curvatureCorrection",
	  coeffDict_,
	  false   // unvalidated yet, switch off by default
      )
    ),
    a1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "a1",
            coeffDict_,
            0.31
        )
    ),
    b1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "b1",
            coeffDict_,
            1.0
        )
    ),
    cr1_ // this class and next two was added by me (model coefficients)
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr1",
            coeffDict_,
            1.0
        )
    ),
    cr2_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr2",
            coeffDict_,
            2.0
        )
    ),
    cr3_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "cr3",
            coeffDict_,
            1.0
        )
    ),
    c1_
    (
        dimensioned<scalar>::lookupOrAddToDict
        (
            "c1",
            coeffDict_,
            10.0
        )
    ),

    nutSmall_("nutSmall", dimLength*dimLength/dimTime, SMALL),
    ySmall_("ySmall", dimLength, SMALL),
#ifndef OF16ext
    omegaSmall_("omegaSmall", omegaMin_.dimensions(), SMALL),
#endif

#if defined(OF301) || defined(OFplus) || defined(OFdev)
    y_(wallDist::New(this->mesh_).y()),
#else
    y_(mesh_),
#endif

    k_
    (
        IOobject
        (
            "k",
            runTime_.timeName(),
            mesh_,
#if defined(OF301) || defined(OFplus) || defined(OFdev)
            IOobject::MUST_READ,
#else
            IOobject::NO_READ,
#endif
            IOobject::AUTO_WRITE
        ),
#if defined(OF301) || defined(OFplus) || defined(OFdev)
	mesh_
#else
        autoCreateK("k", mesh_)
#endif
    ),
    omega_
    (
        IOobject
        (
            "omega",
            runTime_.timeName(),
            mesh_,
#if defined(OF301) || defined(OFplus) || defined(OFdev)
            IOobject::MUST_READ,
#else
            IOobject::NO_READ,
#endif
            IOobject::AUTO_WRITE
        ),
#if defined(OF301) || defined(OFplus) || defined(OFdev)
	mesh_
#else
        autoCreateOmega("omega", mesh_)
#endif
    ),
    nut_
    (
        IOobject
        (
            "nut",
            runTime_.timeName(),
            mesh_,
#if defined(OF301) || defined(OFplus) || defined(OFdev)
            IOobject::MUST_READ,
#else
            IOobject::NO_READ,
#endif
            IOobject::AUTO_WRITE
        ),
#if defined(OF301) || defined(OFplus) || defined(OFdev)
	mesh_
#else
        autoCreateNut("nut", mesh_)
#endif
     ),

   yPlus_
    (
     IOobject
     (
      "yPlus",
      runTime_.timeName(),
      mesh_,
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
      ),
     mesh_,
     dimensionedScalar("yPlus", dimless, 0.0)
     ),
    
    Utau_
    (
     IOobject
     (
      "Utau",
      runTime_.timeName(),
      mesh_,
      IOobject::NO_READ,
      IOobject::AUTO_WRITE
      ),
     mesh_,
     dimensionedScalar("Utau", dimVelocity, 0.0)
     )
{
//     nut_ =
//         a1_*k_
//        /max
//         (
//             a1_*(omega_ + omegaSmall_),
//             b1_*F2()*sqrt(2.0)*mag(symm(fvc::grad(U_)))
//         );
//     nut_.correctBoundaryConditions();

    //correct();
#if defined(OF301) || defined(OFplus) || defined(OFdev)
    printCoeffs("kOmegaSST2");
#else
    printCoeffs();
#endif
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

tmp<volSymmTensorField> kOmegaSST2::R() const
{
    return tmp<volSymmTensorField>
    (
        new volSymmTensorField
        (
            IOobject
            (
                "R",
                runTime_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            ((2.0/3.0)*I)*k_ - nut_*twoSymm(fvc::grad(U_)),
            k_.boundaryField().types()
        )
    );
}


tmp<volSymmTensorField> kOmegaSST2::devReff() const
{
    return tmp<volSymmTensorField>
    (
        new volSymmTensorField
        (
            IOobject
            (
                "devRhoReff",
                runTime_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
           -nuEff()*dev(twoSymm(fvc::grad(U_)))
        )
    );
}


tmp<fvVectorMatrix> kOmegaSST2::divDevReff(
#ifndef Fx40
	  volVectorField& U
#endif
) const
{
#ifdef Fx40
    const volVectorField& U=U_;
#endif
    return
    (
      - fvm::laplacian(nuEff(), U)
      - fvc::div(nuEff()*dev(fvc::grad(U)().T()))
    );
}

#ifndef OF16ext
tmp<fvVectorMatrix> kOmegaSST2::divDevRhoReff
(
    const volScalarField& rho,
    volVectorField& U
) const
{
    volScalarField muEff("muEff", rho*nuEff());

    return
    (
      - fvm::laplacian(muEff, U)
      - fvc::div(muEff*dev(T(fvc::grad(U))))
    );
}
#endif

bool kOmegaSST2::read()
{
    if (RASModel::read())
    {
        alphaK1_.readIfPresent(coeffDict());
        alphaK2_.readIfPresent(coeffDict());
        alphaOmega1_.readIfPresent(coeffDict());
        alphaOmega2_.readIfPresent(coeffDict());
        gamma1_.readIfPresent(coeffDict());
        gamma2_.readIfPresent(coeffDict());
        beta1_.readIfPresent(coeffDict());
        beta2_.readIfPresent(coeffDict());
        betaStar_.readIfPresent(coeffDict());
        a1_.readIfPresent(coeffDict());
        b1_.readIfPresent(coeffDict());
        c1_.readIfPresent(coeffDict());
	cc_.readIfPresent("curvatureCorrection", coeffDict());
	cr1_.readIfPresent(coeffDict());
	cr2_.readIfPresent(coeffDict());
	cr3_.readIfPresent(coeffDict());

        return true;
    }
    else
    {
        return false;
    }
}


void kOmegaSST2::correct()
{
    RASModel::correct();

    if (!turbulence_)
    {
        return;
    }

#if not (defined(OF301) || defined (OFplus) || defined(OFdev))
    if (mesh_.changing())
    {
        y_.correct();
    }
#endif

    volScalarField S2 = magSqr(symm(fvc::grad(U_)));
    volScalarField G("RASModel::G", nut_*2*S2);
    
    
    // Update omega and G at the wall
	
    const fvPatchList& patches = mesh_.boundary();

    //- Initialise the near-wall omega and G fields to zero
    forAll(patches, patchi)
    {
        const fvPatch& curPatch = patches[patchi];

        if
        (
            isA<fixedInternalValueFvPatchScalarField >
            (omega_.boundaryField()[patchi])
        )
        {
            forAll(curPatch, facei)
            {
                label faceCelli = curPatch.faceCells()[facei];

                omega_[faceCelli] = 0.0;
                G[faceCelli] = 0.0;
            }
        }
    }

    omega_.boundaryField().updateCoeffs();

    volScalarField Frot
    (
        IOobject
        (
            "Frot",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        dimensionedScalar("", dimless, 1.0)
    );

    if (cc_)
    {
        tmp<volTensorField> tgradU = fvc::grad(U_);
        tmp<volTensorField> tSkew = -skew(tgradU()); //Was added by me (antisymmetric part of the StressTensor)
        tmp<volSymmTensorField> tSymm = symm(tgradU()); //Was added by me (symmetric part of the StressTensor)
        volScalarField symInnerProduct = max(2. * tSymm() && tSymm(), dimensionedScalar("1e-16", dimensionSet(0, 0, -2, 0, 0), 1e-16) );
        volScalarField asymInnerProduct = max(2. * tSkew() && tSkew(), dimensionedScalar("1e-16", dimensionSet(0, 0, -2, 0, 0), 1e-16) );
        volScalarField rStar = sqrt(symInnerProduct/asymInnerProduct);

        volScalarField D = 
        sqrt(max(symInnerProduct, 0.09*omega_*omega_)); //Possibly wrong. Don't know what Omega is used in equation
        D=max(D, dimensionedScalar("1e-16", D.dimensions(), 1e-16) );

        linear<symmTensor> lis(mesh_);
        tmp<volSymmTensorField> divS =
            fvc::ddt(tSymm())
            + fv::gaussConvectionScheme<symmTensor>
            (
                mesh_,
                phi_,
                tmp<surfaceInterpolationScheme<symmTensor> > (lis)
            ).fvcDiv(phi_, tSymm());

       volScalarField rT = ( tSkew() & tSymm() ) && divS ;

        divS.clear();
        tSkew.clear();
        tSymm.clear();
        volScalarField rTilda = 2. * rT / (sqrt(asymInnerProduct)*D*D*D);
        Frot=
            max
            (
                min
                (

                    (1.0 + cr1_)*2.0*rStar
                    /
                    (1.0 + rStar)
                    *
                    (1.0 - cr3_*atan(cr2_*rTilda))
                    -
                    cr1_,

                    1.25
                ),
                0.0
            );

        if(runTime_.outputTime())
        {
            Frot.write();
        }
        rStar.clear();
        rTilda.clear();
        rT.clear();
        D.clear();
    }

    volScalarField CDkOmega =
        (2*alphaOmega2_)*(fvc::grad(k_) & fvc::grad(omega_))/omega_;

    volScalarField F1 = this->F1(CDkOmega);

    // Turbulent frequency equation
    tmp<fvScalarMatrix> tomegaEqn
    (
        fvm::ddt(omega_)
        + fvm::div(phi_, omega_)
        - fvm::laplacian(DomegaEff(F1), omega_)
        ==
        Frot*gamma(F1)*min(G, c1_*betaStar_*k_*omega_)/max(nut_, nutSmall_)
        - fvm::Sp(beta(F1)*omega_, omega_)
        - fvm::SuSp
        (
            (F1 - scalar(1))*CDkOmega/omega_,
            omega_
        )
    );
    
    fvScalarMatrix& omegaEqn = UNIOF_TMP_NONCONST(tomegaEqn);

    omegaEqn.relax();

#if !(defined(Fx31)||defined(Fx32)||defined(Fx40))
    omegaEqn.boundaryManipulate(UNIOF_BOUNDARY_NONCONST(omega_));
#endif
    
    solve(tomegaEqn);
#ifdef OF16ext
    bound(omega_, omega0_);
#else
    bound(omega_, omegaMin_);
#endif

    // Turbulent kinetic energy equation
    tmp<fvScalarMatrix> tkEqn
    (
        fvm::ddt(k_)
      + fvm::div(phi_, k_)
      - fvm::laplacian(DkEff(F1), k_)
     ==
        Frot*min(G, c1_*betaStar_*k_*omega_)
      - fvm::Sp(betaStar_*omega_, k_)
    );
    fvScalarMatrix& kEqn = UNIOF_TMP_NONCONST(tkEqn);

    kEqn.relax();
    solve(tkEqn);
#ifdef OF16ext
    bound(k_, k0_);
#else
    bound(k_, kMin_);
#endif

    // Re-calculate viscosity
    nut_ = a1_*k_/max(a1_*omega_, b1_*F2()*sqrt(S2));
    nut_.correctBoundaryConditions();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
#if not (defined(OF301) || defined(OFplus) || defined(OFdev))
} // End namespace incompressible
#endif
} // End namespace Foam

// ************************************************************************* //
