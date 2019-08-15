/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2013 OpenFOAM Foundation
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

Class
    Foam::mappedWithSwirlFixedValueFvPatchVectorField

Group
    grpGenericBoundaryConditions grpCoupledBoundaryConditions

Description
    This boundary condition maps the value at a set of cells or patch faces
    back to *this.  

    The sample mode is set by the underlying mapping engine, provided by the
    mappedPatchBase class.
    
    \heading Patch usage

    \table
        Property     | Description             | Required    | Default value
        fieldName    | name of field to be mapped | no       | this field name
        setAverage   | flag to activate setting of average value | yes |
        average      | average value to apply if \c setAverage = yes | yes |
        interpolationScheme | type of interpolation scheme | no |
    \endtable

    Example of the boundary condition specification:
    \verbatim
    myPatch
    {
        type            mapped;
        fieldName       T;
        setAverage      no;
        average         0;
        interpolationScheme cell;
        value           uniform 0;
    }
    \endverbatim

    When employing the \c nearestCell sample mode, the user must also specify
    the interpolation scheme using the \c interpolationScheme entry.

    In case of interpolation (where scheme != cell) the limitation is that
    there is only one value per cell.  For example, if you have a cell with two
    boundary faces and both faces sample into the cell, both faces will get the
    same value.

Note
    It is not possible to sample internal faces since volume fields are not
    defined on faces.

SeeAlso
    Foam::mappedPatchBase
    Foam::interpolation
    Foam::fixedValueFvPatchField

SourceFiles
    mappedWithSwirlFixedValueFvPatchVectorField.C

\*---------------------------------------------------------------------------*/

#ifndef mappedWithSwirlFixedValueFvPatchVectorField_H
#define mappedWithSwirlFixedValueFvPatchVectorField_H

#include "fixedValueFvPatchFields.H"
//#include "interpolation.H"
#include "mappedPatchFieldBase.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                Class mappedWithSwirlFixedValueFvPatchVectorField Declaration
\*---------------------------------------------------------------------------*/

class mappedWithSwirlFixedValueFvPatchVectorField
:
    public fixedValueFvPatchField<vector>,
    public mappedPatchFieldBase<vector>
{

protected:
  
  point p0_;
  vector ax_;
  scalar S_targ_;

    // Protected Member Functions

        const mappedPatchBase& mapper
        (
            const fvPatch& p,
            const DimensionedField<vector, volMesh>& iF
        );


public:

    //- Runtime type information
    TypeName("mappedWithSwirl");


    // Constructors

        //- Construct from patch and internal field
        mappedWithSwirlFixedValueFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct from patch, internal field and dictionary
        mappedWithSwirlFixedValueFvPatchVectorField
        (
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const dictionary&
        );

        //- Construct by mapping given mappedWithSwirlFixedValueFvPatchVectorField
        //  onto a new patch
        mappedWithSwirlFixedValueFvPatchVectorField
        (
            const mappedWithSwirlFixedValueFvPatchVectorField&,
            const fvPatch&,
            const DimensionedField<vector, volMesh>&,
            const fvPatchFieldMapper&
        );

        //- Construct as copy
        mappedWithSwirlFixedValueFvPatchVectorField
        (
            const mappedWithSwirlFixedValueFvPatchVectorField&
        );

        //- Construct and return a clone
        virtual tmp<fvPatchField<vector> > clone() const
        {
            return tmp<fvPatchField<vector> >
            (
                new mappedWithSwirlFixedValueFvPatchVectorField(*this)
            );
        }

        //- Construct as copy setting internal field reference
        mappedWithSwirlFixedValueFvPatchVectorField
        (
            const mappedWithSwirlFixedValueFvPatchVectorField&,
            const DimensionedField<vector, volMesh>&
        );

        //- Construct and return a clone setting internal field reference
        virtual tmp<fvPatchField<vector> > clone
        (
            const DimensionedField<vector, volMesh>& iF
        ) const
        {
            return tmp<fvPatchField<vector> >
            (
                new mappedWithSwirlFixedValueFvPatchVectorField(*this, iF)
            );
        }


    // Member functions

        // Evaluation functions

            //- Update the coefficients associated with the patch field
            virtual void updateCoeffs();

        //- Write
        virtual void write(Ostream&) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
