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

Class
    Foam::incompressible::RASModels::kOmegaHe

Description
    EARSMko2005a Model in NASA classification
    
    For reference see 
    
    Hellsten, A. New advanced k-w turbulence model for high-lift aerodynamics 
    
    or 
    
    http://turbmodels.larc.nasa.gov/easmko.html
    
    The default model coefficients correspond to the following:
    @verbatim
        kOmegaHeCoeffs
        {
        }
    @endverbatim

SourceFiles
    kOmegaHe.C
    
    Implemented by Thomas Baumann, KIT, 2010: thomas.baumann@freenet.de
    Corrected by Ivan Shevchuk, Uni-Rostock, LeMoS, 2014

\*---------------------------------------------------------------------------*/

#ifndef kOmegaHe_H
#define kOmegaHe_H

#include "RASModel.H"
#include "wallDist.H"	//wird für Wandfunktionen benötigt
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace incompressible
{
namespace RASModels
{

/*---------------------------------------------------------------------------*\
                           Class kOmegaHe Declaration
\*---------------------------------------------------------------------------*/

class kOmegaHe
:
    public RASModel
{
    // Private data

        // Model coefficients

        // Fields

        volScalarField k_;
        volScalarField omega_;
        volScalarField nut_;
        volScalarField Cmu_;
        volScalarField tau_;
        
	    volSymmTensorField R_;
	    wallDist y_;
        
        volTensorField nonlinear_;
        
        dimensionedScalar   betastar;
        dimensionedScalar   beta1a;
        dimensionedScalar   beta2a;
        dimensionedScalar   sigmad1;
        dimensionedScalar   sigmad2;
        dimensionedScalar   sigmak1;
        dimensionedScalar   sigmak2;
        dimensionedScalar   sigmaw1;
        dimensionedScalar   sigmaw2;
        dimensionedScalar   gamma1;
        dimensionedScalar   gamma2;
        
        dimensionedScalar   ke;    
        dimensionedScalar   nlCoeff;
        scalar Cmix;  
        scalar Ctau;    



public:

    //- Runtime type information
    TypeName("kOmegaHe");

    // Constructors

        //- Construct from components
        kOmegaHe
        (
            const volVectorField& U,
            const surfaceScalarField& phi,
            transportModel& transport,
   	        const word& turbulenceModelName = turbulenceModel::typeName,
            const word& modelName = typeName
        );


    //- Destructor
    virtual ~kOmegaHe()
    {}


    // Member Functions

        //- Return the turbulence viscosity
        virtual tmp<volScalarField> nut() const
        {
            return nut_;
        }

        //- Return the effective diffusivity for epsilon
        tmp<volScalarField> DepsilonEff() const
        {
            return tmp<volScalarField>
            (
                new volScalarField("DepsilonEff", nut_ + nu())
            );
        }
        
        
        //- Return the effective diffusivity for k
        tmp<volScalarField> DkEff() const
        {
            return tmp<volScalarField>
            (
                new volScalarField("DkEff", nut_ + nu())//das hier ist falsch!!! es fehlt nut_*sigmak
            );
        }

        //- Return the effective diffusivity for omega
        tmp<volScalarField> DomegaEff() const
        {
            return tmp<volScalarField>
            (
                new volScalarField("DomegaEff", nut_ + nu())	//nut_*sigmaw fehlt!!!
            );
        }
        

        //- Return the turbulence kinetic energy
        virtual tmp<volScalarField> k() const
        {
            return k_;
        }

        //- Return the turbulence specific dissipation rate
        virtual tmp<volScalarField> omega() const
        {
            return omega_;
	    
	    
	    
        }

        //
        /*virtual tmp<volSymmTensorField> nonlinear() const
        {
            return nonlinear_;
        }*/

        //- Return the turbulence kinetic energy dissipation rate
        virtual tmp<volScalarField> epsilon() const
        {
            return tmp<volScalarField>
            (
                new volScalarField
                (
                    IOobject
                    (
                        "epsilon",
                        mesh_.time().timeName(),
                        mesh_
                    ),
                    betastar*k_*omega_,//beta*k_*omega_,//hier fehlt beta *k_*omega_
                    omega_.boundaryField().types()
                )
            );
        }
        //- Return the Reynolds stress tensor
        //virtual tmp<volSymmTensorField> R() const;

        //- Return the Reynolds stress tensor	//modified fuer ASM-model
        virtual tmp<volSymmTensorField> R() const
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
                    R_,
                    k_.boundaryField().types()
                )
            );
        }					//until now
	

        //- Return the effective stress tensor including the laminar stress
        virtual tmp<volSymmTensorField> devReff() const;

        //- Return the source term for the momentum equation
        virtual tmp<fvVectorMatrix> divDevReff(volVectorField& U) const;
        
        
        virtual tmp<fvVectorMatrix> divDevRhoReff
        ( 
            const volScalarField& rho, 
            volVectorField& U 
        ) const ;
	

        //- Solve the turbulence equations and correct the turbulence viscosity
        virtual void correct();

        //- Read RASProperties dictionary
        virtual bool read();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace incompressible
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
