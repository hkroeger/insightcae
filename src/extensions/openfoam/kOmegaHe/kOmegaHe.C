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

    //the model implemented by Thomas Baumann in 2010: thomas.baumann@freenet.de
    
\*---------------------------------------------------------------------------*/

#include "kOmegaHe.H"
#include "addToRunTimeSelectionTable.H"
#include "backwardsCompatibilityWallFunctions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace incompressible
{
namespace RASModels
{

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

defineTypeNameAndDebug(kOmegaHe, 0);
addToRunTimeSelectionTable(RASModel, kOmegaHe, dictionary);

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

kOmegaHe::kOmegaHe
(
    const volVectorField& U,
    const surfaceScalarField& phi,
    transportModel& transport,
    const word& turbulenceModelName,
    const word& modelName
)
:
    RASModel(modelName, U, phi, transport, turbulenceModelName),
    k_
    (
        IOobject
        (
            "k",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateK("k", mesh_)
    ),
    
    omega_
    (
        IOobject
        (
            "omega",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateOmega("omega", mesh_)
    ),
    
    nut_
    (
        IOobject
        (
            "nut",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        autoCreateNut("nut", mesh_)
    ),
    
    Cmu_
    (
        IOobject
        (
            "nut",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        mesh_,
        0.09
    ),
    
    tau_
    (
        IOobject
        (
            "nut",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        1/(0.09*omega_)
    ),
    
    R_	//new
    (
        IOobject
        (
            "R",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        2*nut_*symm(fvc::grad(U))()
    ),
    
    y_(mesh_),
    
    nonlinear_
    (
        IOobject
        (
            "nonlinear",
            runTime_.timeName(),
            mesh_,
            IOobject::NO_READ,
            IOobject::AUTO_WRITE
        ),
        2*nut_*fvc::grad(U)()
        
    ),
    betastar 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "betaStar",
            coeffDict_,
            0.09
        )
    ),
    beta1a 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta1a",
            coeffDict_,
            0.0747
        )
    ),
    beta2a 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "beta2a",
            coeffDict_,
            0.0828
        )
    ),
    sigmad1
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmad1",
            coeffDict_,
            1.0
        )
    ),
    sigmad2 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmad2",
            coeffDict_,
            0.4
        )
    ),
    sigmak1
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmak1",
            coeffDict_,
            1.1
        )
    ),
    sigmak2 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmak2",
            coeffDict_,
            1.1
        )
    ),
    sigmaw1 
    ( 
         dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmaw1",
            coeffDict_,
            0.53
        )
    ),
    sigmaw2 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "sigmaw2",
            coeffDict_,
            1.0
        )
    ),
    gamma1 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma1",
            coeffDict_,
            (beta1a/betastar - sqr(0.42)*sigmaw1/sqrt(betastar)).value()
        )
    ),
    gamma2 
    ( 
        dimensioned<scalar>::lookupOrAddToDict
        (
            "gamma2",
            coeffDict_,
            0.44//(beta2a/betastar - sqr(0.42)*sigmaw2/sqrt(betastar)).value()
        )
    ),
    ke
    (   
        "kinf",
        dimensionSet(0,2,-2,0,0,0,0),
        dimensioned<scalar>::lookupOrAddToDict
        (
            "kinf",
            coeffDict_,
            1e-08//(beta2a/betastar - sqr(0.42)*sigmaw2/sqrt(betastar)).value()
        ).value()
    ),
    nlCoeff
    (   
        dimensioned<scalar>::lookupOrAddToDict
        (
            "nlCoeff",
            coeffDict_,
            1//(beta2a/betastar - sqr(0.42)*sigmaw2/sqrt(betastar)).value()
        )
    ),
    Cmix ( 1.5 ),
    Ctau ( 6.0 )
{
  nonlinear_=calcNonLinearPart(fvc::grad(U));
  nut_=Cmu_ * k_ * tau_;
  R_= ((2.0/3.0)*I*k_)- 2*nuEff()*symm(fvc::grad(U)) + symm(nonlinear_);
  
  printCoeffs();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

tmp<volSymmTensorField> kOmegaHe::devReff() const
{
    return tmp<volSymmTensorField>
    (
        new volSymmTensorField
        (
            IOobject
            (
                "devReff",
                runTime_.timeName(),
                mesh_,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            - nuEff()*twoSymm(fvc::grad(U_)) + symm(nonlinear_)
        )
    );
}

// ((2.0/3.0)*I*k_)- 2*nuEff()*S_ + symm(nonLinearPart)

tmp<fvVectorMatrix> kOmegaHe::divDevReff(volVectorField& U) const
{
    return
    (   
      - fvm::laplacian(nuEff(), U)
      - fvc::div(nuEff()*T(fvc::grad(U)))
      + fvc::div(nonlinear_)
    );
}

tmp<fvVectorMatrix> kOmegaHe::divDevRhoReff
(
    const volScalarField& rho,
    volVectorField& U
) const
{
    volScalarField muEff("muEff", rho*nuEff());

    return
    (   
      - fvm::laplacian(muEff, U)
      - fvc::div(muEff*T(fvc::grad(U)))
      + fvc::div(rho*nonlinear_)
    );
}

/*tmp<volSymmTensorField> kOmegaHe::R() const
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
    );*/



bool kOmegaHe::read()
{
    if (RASModel::read())
    {
        return true;
    }
    else
    {
        return false;
    }
}

tmp<volTensorField> kOmegaHe::calcNonLinearPart(const volTensorField& gradU)
{
    volTensorField         W_    = - skew(gradU);
    volSymmTensorField     S_    =  symm(gradU); // dimensioned
    
    volScalarField     tau1 = 1.0/(omega_*betastar);
    volScalarField     tau2 = Ctau*sqrt(nu() /(betastar*k_*omega_));
    tau_ = max(tau1, tau2);
    
    volSymmTensorField S = tau_*S_; // non-dim
    volTensorField     W = tau_*W_;
    

    //!Invariants are traces of S*S , S*S*S, W*W, S*W*W , ... , not the double inner products
    //!See 
    //!Gatski, Jongen, Nonlinear eddy viscosity and algebraic stress models for solving complex turbulent flows
    //!or
    //!Wallin, Johansson, A 
    //! 
    
    volScalarField     IIs  = tr(S&S);     
    volScalarField     IIIs = tr(S&S&S);  
    volScalarField     IIw  = tr(W&W);     
    volScalarField     IV   = tr(S&W&W) ;   
    volScalarField     V    = tr(S&S&W&W) ;     
    
    dimensionedScalar  Neq     =  81.0/20.0;
    dimensionedScalar  Cdiff   =  2.2;
    volScalarField     beta1eq =  -(6.0/5.0) * Neq / (sqr(Neq)-2.0*IIw);
    
    volScalarField     A3stitch = 9.0/5.0 + 9.0/4.0*Cdiff*max(1.0 + beta1eq*IIs, 0.0);
    
    volScalarField     P1 = A3stitch * ( sqr(A3stitch)/27.0 + (9.0/20.0)*IIs - (2.0/3.0)*IIw );	
    volScalarField     P2 = sqr(P1) - pow( sqr(A3stitch)/9.0 + (9.0/10.0)*IIs + (2.0/3.0)*IIw , 3.0 );
    
    volScalarField     term_acos =  P1/( sqrt( mag(sqr(P1) - P2) ) + 1e-12 );
    volScalarField     term_acos_corrected = max( ( min( term_acos , 1.0 ) ),-1.0); 
    
    //----------------------------------------------------------------------------------------------------------------------------------
    
    volScalarField     Nc_ = 
                       
                       pos(P2) * ( ((A3stitch/3.0) + pow(mag(P1 + sqrt(mag(P2))),(1.0/3.0)) 
                       + sign(P1-sqrt(mag(P2)))*pow(mag(P1 - sqrt(mag(P2))),(1.0/3.0))))
                       
                       +
                       
                       neg(P2) * ( A3stitch/3.0 + 2.0 * pow( mag(sqr(P1) - P2), 1.0/6.0 ) * cos((1.0/3.0)*acos(term_acos_corrected)) )  ;
                       
    //----------------------------------------------------------------------------------------------------------------------------------
    
    volScalarField     N_ = 
    
                       Nc_ 
                       
                       + 
                       
                       ( 162.0 * ( sqr(IV) + ( V - 0.5*IIs*IIw ) * sqr(Nc_) ) )
                       /
                       ( 20.0 * pow( Nc_,4.0 )*( Nc_ - 0.5*A3stitch ) - IIw*( 10.0*pow(Nc_,3) + 15.0*A3stitch*sqr(Nc_)) + 10.0*A3stitch*sqr(IIw) );
                    
    //----------------------------------------------------------------------------------------------------------------------------------
        
    volScalarField     Q = (5.0/6.0) * ( sqr(N_) - 2.0*IIw ) * ( 2.0*sqr(N_) - IIw );
    
    volScalarField     beta1 = - N_ * ( 2.0*sqr(N_)-7.0*IIw ) / Q; 
    volScalarField     beta3 = - 12.0 * IV/(N_*Q);
    volScalarField     beta4 = - 2.0 * (sqr(N_)-2.0*IIw)/Q; 
    volScalarField     beta6 = - 6.0 * N_/Q;
    volScalarField     beta9 =   6.0 / Q;

    volTensorField     SW   = S & W;
    volTensorField     WS   = W & S;
    volTensorField     WW   = W & W;
    volTensorField     SWW  = S & WW;
    volTensorField     WWS  = WW & S;
    volTensorField     WSWW = WS & WW;
    volTensorField     WWSW = WW & SW;
    
    
    

//     volTensorField      nonLinearPart =
//     k_*nlCoeff*
//     (
//         beta3*(WW-1.0/3.0*IIw*I) 
//       + beta4*(SW-WS) 
//       + beta6*(SWW + WWS - IIw*S -2.0/3.0*IV*I) 
//       + beta9*(WSWW-WWSW)
//     );

    Cmu_ = -0.5 * ( beta1  + nlCoeff*IIw*beta6 );
    
    return tmp<volTensorField>(new volTensorField
    (
      k_*nlCoeff*
      (
	  beta3*(WW-1.0/3.0*IIw*I) 
	+ beta4*(SW-WS) 
	+ beta6*(SWW + WWS - IIw*S -2.0/3.0*IV*I) 
	+ beta9*(WSWW-WWSW)
      )      
    ));
}

void kOmegaHe::correct()
{
    RASModel::correct();

    if (!turbulence_)
    {
        return;
    }
    
    volTensorField         gradU =  fvc::grad(U_)();
    volSymmTensorField     S_    =  symm(gradU); // dimensioned
    
    nut_ = Cmu_ * k_ * tau_;
			
    volSymmTensorField  linearPart = 2.0 * nut_ * S_ ;
    volTensorField      nonLinearPart = calcNonLinearPart(gradU);
    
    volSphericalTensorField kpart = (2.0/3.0)*I*k_;
        
    volScalarField G ( GName(), (linearPart - nonLinearPart - kpart) && S_);
     
    //volScalarField G ( GName(), linearPart && S_ ); // S_.T() - possible 
    scalar yplusfactor = 1; 
    #include "correctwnew.H"  
    
    //omega_.boundaryField().updateCoeffs();
    
    volScalarField  Gamma1 = sqrt(k_)/(betastar*omega_*y_);
    volScalarField  Gamma2 = 500.0*nu()/(omega_*sqr(y_));
    volScalarField  Gamma3 = ( 20*k_/max(sqr(y_)*(fvc::grad(k_) & fvc::grad(omega_))/omega_,200.0*ke));
    volScalarField  Gamma  = ( min ( max(Gamma1, Gamma2), Gamma3 ) );
    
    volScalarField  fmix = tanh(Cmix*pow(Gamma,4));
    
    
    volScalarField gamma  = fmix*gamma1  + (1.0-fmix)*gamma2;
    volScalarField beta   = fmix*beta1a  + (1.0-fmix)*beta2a;
    volScalarField sigmak = fmix*sigmak1 + (1.0-fmix)*sigmak2;
    volScalarField sigmaw = fmix*sigmaw1 + (1.0-fmix)*sigmaw2;
    volScalarField sigmad = fmix*sigmad1 + (1.0-fmix)*sigmad2;

    dimensionedScalar dimnull("dimnull",dimensionSet (0,0,-3,0,0,0,0),0.0);
    
    tmp<fvScalarMatrix> omegaEqn
    (
        fvm::ddt(omega_)
      + fvm::div(phi_, omega_)
      //~ - fvm::Sp(fvc::div(phi_), omega_)
      - fvm::laplacian(sigmaw*nut_ + nu(), omega_)
      + fvm::Sp(beta*omega_, omega_)
     ==
        gamma * G * omega_ / k_
      + sigmad/omega_*max(fvc::grad(k_)&fvc::grad(omega_) ,dimnull)
     );

    
    omegaEqn().relax();

    solve(omegaEqn);
    bound(omega_, omegaMin_);
    
    // Turbulent kinetic energy equation
    tmp<fvScalarMatrix> kEqn
    (
        fvm::ddt(k_)
      + fvm::div(phi_, k_)
      //~ - fvm::Sp(fvc::div(phi_), k_)
      - fvm::laplacian(sigmak*nut_ + nu(), k_)
      + fvm::Sp(betastar*omega_, k_)
     ==
        G
      
    );
  
    kEqn().relax();
    solve(kEqn);
    bound(k_, kMin_);

    nonlinear_ = nonLinearPart; // symm(nonlineara)
    
//     Cmu = -0.5 * ( beta1 + nlCoeff*IIw*beta6 );
    nut_ = Cmu_ * k_ * tau_;
    
    
    //~ volScalarField nonlin = magSqr(fvc::div(nonlinear_));
    //~ nonlin.write();
    
    //~ Info << "Nonlinear term max = " << gMax(nonlin) << " min= " << gMin(nonlin) << endl ;
    
    R_= ((2.0/3.0)*I*k_)- 2*nuEff()*S_ + symm(nonLinearPart);
    
    nut_.correctBoundaryConditions();
    
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace incompressible
} // End namespace Foam

// ************************************************************************* //
