#ifndef KOMEGASST3_H
#define KOMEGASST3_H
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
    Foam::incompressible::RASModels::kOmegaSST

Description
    Implementation of the k-omega-SST turbulence model for incompressible
    flows.

    Turbulence model described in:
    @verbatim
        Menter, F., Esch, T.
        "Elements of Industrial Heat Transfer Prediction"
        16th Brazilian Congress of Mechanical Engineering (COBEM),
        Nov. 2001
    @endverbatim

    Note that this implementation is written in terms of alpha diffusion
    coefficients rather than the more traditional sigma (alpha = 1/sigma) so
    that the blending can be applied to all coefficuients in a consistent
    manner.  The paper suggests that sigma is blended but this would not be
    consistent with the blending of the k-epsilon and k-omega models.

    Also note that the error in the last term of equation (2) relating to
    sigma has been corrected.

    Wall-functions are applied in this implementation by using equations (14)
    to specify the near-wall omega as appropriate.

    The blending functions (15) and (16) are not currently used because of the
    uncertainty in their origin, range of applicability and that is y+ becomes
    sufficiently small blending u_tau in this manner clearly becomes nonsense.

    The default model coefficients correspond to the following:
    @verbatim
        kOmegaSST_EngysCoeffs
        {
            alphaK1     0.85034;
            alphaK2     1.0;
            alphaOmega1 0.5;
            alphaOmega2 0.85616;
            beta1       0.075;
            beta2       0.0828;
            betaStar    0.09;
            gamma1      0.5532;
            gamma2      0.4403;
            a1          0.31;
            c1          10.0;
        }
    @endverbatim

SourceFiles
    kOmegaSST3.C

\*---------------------------------------------------------------------------*/


#include "RASModel.H"
#include "wallDist.H"

#if OF_VERSION>=030000
#include "IncompressibleTurbulenceModel.H"
#include "incompressible/transportModel/transportModel.H"
#include "eddyViscosity.H"
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

#if OF_VERSION>=030000
typedef IncompressibleTurbulenceModel<transportModel> transportModelIncompressibleTurbulenceModel;
typedef eddyViscosity<RASModel<transportModelIncompressibleTurbulenceModel> > RAStransportModelIncompressibleTurbulenceModel;
#endif

#if OF_VERSION<030000
namespace incompressible
{
#endif

namespace RASModels
{

/*---------------------------------------------------------------------------*\
                           Class kOmega Declaration
\*---------------------------------------------------------------------------*/

class kOmegaSST3
:
#if OF_VERSION>=030000 //defined(OF301) || defined(OFplus) || defined(OFdev)||defined(OFesi1806)
    public RAStransportModelIncompressibleTurbulenceModel
#else
    public RASModel
#endif
{
    // Private data

        // Model coefficients
            dimensionedScalar alphaK1_;
            dimensionedScalar alphaK2_;

            dimensionedScalar alphaOmega1_;
            dimensionedScalar alphaOmega2_;

            dimensionedScalar gamma1_;
            dimensionedScalar gamma2_;

            dimensionedScalar beta1_;
            dimensionedScalar beta2_;

            dimensionedScalar betaStar_;

            dimensionedScalar a1_;
            dimensionedScalar b1_;
            dimensionedScalar c1_;

            //- Flag to include the F3 term
            Switch F3_;

        //- Wall distance field
        //  Note: different to wall distance in parent RASModel
#if OF_VERSION>=030000 //defined(OF301) || defined(OFplus) || defined(OFdev)||defined(OFesi1806)
        const volScalarField& y_;
#else
        wallDist y_;
#endif

        volScalarField k_;
        volScalarField omega_;


    // Decay control

        //- Flag to include the decay control
        Switch decayControl_;
        dimensionedScalar kInf_;
        dimensionedScalar omegaInf_;

        word freeSurfaceAlphaName_;


        // Protected Member Functions

            void setDecayControl(const dictionary& dict);

            virtual tmp<volScalarField> F1(const volScalarField& CDkOmega) const;
            virtual tmp<volScalarField> F2() const;
            virtual tmp<volScalarField> F3() const;
            virtual tmp<volScalarField> F23() const;

            tmp<volScalarField> blend
            (
                const volScalarField& F1,
                const dimensionedScalar& psi1,
                const dimensionedScalar& psi2
            ) const
            {
                return F1*(psi1 - psi2) + psi2;
            }

            tmp<volScalarField::Internal> blend
            (
                const volScalarField::Internal& F1,
                const dimensionedScalar& psi1,
                const dimensionedScalar& psi2
            ) const
            {
                return F1*(psi1 - psi2) + psi2;
            }

            tmp<volScalarField> alphaK(const volScalarField& F1) const
            {
                return blend(F1, alphaK1_, alphaK2_);
            }

            tmp<volScalarField> alphaOmega(const volScalarField& F1) const
            {
                return blend(F1, alphaOmega1_, alphaOmega2_);
            }

            tmp<volScalarField::Internal> beta
            (
                const volScalarField::Internal& F1
            ) const
            {
                return blend(F1, beta1_, beta2_);
            }

            tmp<volScalarField::Internal> gamma
            (
                const volScalarField::Internal& F1
            ) const
            {
                return blend(F1, gamma1_, gamma2_);
            }

            virtual void correctNut(const volScalarField& S2);

            virtual void correctNut();

            //- Return k production rate
            virtual tmp<volScalarField::Internal> Pk
            (
                const volScalarField::Internal& G
            ) const;

            //- Return epsilon/k which for standard RAS is betaStar*omega
            virtual tmp<volScalarField::Internal> epsilonByk
            (
                const volScalarField& F1,
                const volTensorField& gradU
            ) const;

            //- Return G/nu
            virtual tmp<volScalarField::Internal> GbyNu
            (
                const volScalarField::Internal& GbyNu0,
                const volScalarField::Internal& F2,
                const volScalarField::Internal& S2
            ) const;

            virtual tmp<fvScalarMatrix> kSource() const;

            virtual tmp<fvScalarMatrix> omegaSource() const;

            virtual tmp<fvScalarMatrix> Qsas
            (
                const volScalarField::Internal& S2,
                const volScalarField::Internal& gamma,
                const volScalarField::Internal& beta
            ) const;


    public:
        TypeName("kOmegaSST3");

//        typedef typename BasicEddyViscosityModel::alphaField alphaField;
//        typedef typename BasicEddyViscosityModel::rhoField rhoField;
//        typedef typename BasicEddyViscosityModel::transportModel transportModel;


        // Constructors

            //- Construct from components
            kOmegaSST3
            (
        #if OF_VERSION>=030000 //defined(OF301) || defined(OFplus) || defined(OFdev)||defined(OFesi1806)
                    const alphaField& alpha,
                    const rhoField& rho,
                    const volVectorField& U,
                    const surfaceScalarField& alphaRhoPhi,
                    const surfaceScalarField& phi,
                    const transportModel& transport,
                    const word& propertiesName = turbulenceModel::propertiesName,
                    const word& type = typeName
        #else
                    const volVectorField& U,
                    const surfaceScalarField& phi,
                    transportModel& transport
        #if OF_VERSION>=010700 || (defined(OF_FORK_extend)&&OF_VERSION>=010604) //!defined(OF16ext) || defined(Fx41)
                                ,
                    const word& turbulenceModelName = turbulenceModel::typeName,
                    const word& modelName = typeName
        #endif
        #endif
            );


        //- Destructor
        virtual ~kOmegaSST3()
        {}


        // Member Functions

            //- Re-read model coefficients if they have changed
            virtual bool read();

            //- Return the effective diffusivity for k
            tmp<volScalarField> DkEff(const volScalarField& F1) const
            {
                return tmp<volScalarField>
                (
                    new volScalarField("DkEff", alphaK(F1)*this->nut_ + this->nu())
                );
            }

            //- Return the effective diffusivity for omega
            tmp<volScalarField> DomegaEff(const volScalarField& F1) const
            {
                return tmp<volScalarField>
                (
                    new volScalarField
                    (
                        "DomegaEff",
                        alphaOmega(F1)*this->nut_ + this->nu()
                    )
                );
            }

            //- Return the turbulence kinetic energy
            virtual tmp<volScalarField> k() const
            {
                return k_;
            }

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
                            this->mesh_.time().timeName(),
                            this->mesh_
                        ),
                        betaStar_*k_*omega_,
                        omega_.boundaryField().types()
                    )
                );
            }

            //- Return the turbulence kinetic energy dissipation rate
            virtual tmp<volScalarField> omega() const
            {
                return omega_;
            }

            //- Solve the turbulence equations and correct the turbulence viscosity
            virtual void correct();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
#if OF_VERSION<030000 //not (defined(OF301) || defined(OFplus) || defined(OFdev)||defined(OFesi1806))
} // namespace incompressible
#endif
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#endif // KOMEGASST3_H
