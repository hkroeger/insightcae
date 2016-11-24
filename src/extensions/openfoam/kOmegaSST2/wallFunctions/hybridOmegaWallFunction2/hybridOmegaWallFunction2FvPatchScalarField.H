/*---------------------------------------------------------------------------*\ 
| File modified by Engys Ltd 2010                                             |
\*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2008-2009 OpenCFD Ltd.
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
    Foam::incompressible::RASModels::hybridOmegaWallFunction2FvPatchScalarField

Description
    Provides a hybrid wall function boundary condition/constraint on omega
	based on the STAR-CD implementation

SourceFiles
    hybridOmegaWallFunction2FvPatchScalarField.C

\*---------------------------------------------------------------------------*/

#ifndef hybridOmegaWallFunction2FvPatchScalarField_H
#define hybridOmegaWallFunction2FvPatchScalarField_H

#include "fixedInternalValueFvPatchField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
#if not defined(OFplus)
namespace incompressible
{
#endif
namespace RASModels
{

/*---------------------------------------------------------------------------*\
           Class hybridOmegaWallFunction2FvPatchScalarField Declaration
\*---------------------------------------------------------------------------*/

class hybridOmegaWallFunction2FvPatchScalarField
:
    public fixedInternalValueFvPatchField<scalar>
{
    // Private data

        //- Name of velocity field
        word UName_;

        //- Name of turbulence kinetic energy field
        word kName_;

        //- Name of turbulence generation field
        word GName_;

        //- Name of laminar viscosity field
        word nuName_;

        //- Name of turbulent viscosity field
        word nutName_;

        //- Cmu coefficient
        scalar Cmu_;

        //- Von Karman constant
        scalar kappa_;

        //- E coefficient
        scalar E_;

        //- tw coefficient
        scalar tw_;

        //- beta1 coefficient
        scalar beta1_;

        //- beta1 coefficient
        scalar betaStar_;


    // Private member functions

        //- Check the type of the patch
        void checkType();

    // Private data
		
		//- wall face weighting for internal field contributions
		mutable Field<scalar>* faceWeightsPtr_;

    // Private member functions
	
		//- calculate boundary face weights of fixedInternalValue boundaries
		void patchTypeFaceWeighting() const;

public:

    //- Runtime type information
    TypeName("hybridOmegaWallFunction2");


    // Constructors

        //- Construct from patch and internal field
        hybridOmegaWallFunction2FvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        hybridOmegaWallFunction2FvPatchScalarField
        (
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given
        // hybridOmegaWallFunction2FvPatchScalarField
        //  onto a new patch
        hybridOmegaWallFunction2FvPatchScalarField
        (
            const hybridOmegaWallFunction2FvPatchScalarField&,
            const fvPatch&,
            const DimensionedField<scalar, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        hybridOmegaWallFunction2FvPatchScalarField
        (
            const hybridOmegaWallFunction2FvPatchScalarField&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchScalarField> clone() const
        {
            return tmp<fvPatchScalarField>
            (
                new hybridOmegaWallFunction2FvPatchScalarField(*this)
            );
        }

        //- Construct as copy setting internal field reference
        hybridOmegaWallFunction2FvPatchScalarField
        (
            const hybridOmegaWallFunction2FvPatchScalarField&,
            const DimensionedField<scalar, volMesh>&
        );

       ~hybridOmegaWallFunction2FvPatchScalarField();

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchScalarField> clone
        (
            const DimensionedField<scalar, volMesh>& iF
        ) const
        {
            return tmp<fvPatchScalarField>
            (
                new hybridOmegaWallFunction2FvPatchScalarField(*this, iF)
            );
        }


    // Member functions
  // Access
  
  //- return const access to face weights
  const Field<scalar>& faceWeights() const;

        // Evaluation functions

            //- Update the coefficients associated with the patch field
            virtual void updateCoeffs();


        // I-O

            //- Write
            void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
#if not defined(OFplus)
} // End namespace incompressible
#endif
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
