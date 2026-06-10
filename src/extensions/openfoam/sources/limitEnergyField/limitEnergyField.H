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

Class
    Foam::fv::limitEnergyField

Description
    Constrains the energy equation to enforce a minimum temperature.

    In cells where T < Tmin, the energy equation row is replaced with the
    hard constraint he = he(p, Tmin), identical in mechanism to
    fixedTemperatureConstraint but applying a lower bound rather than a
    fixed value.  This fires inside every PIMPLE outer iteration and every
    energy corrector, preventing negative temperatures from persisting across
    corrector calls.

Usage
    Minimal example in system/fvOptions:
    \verbatim
    limitEnergyGas
    {
        type            limitEnergyField;
        active          yes;
        selectionMode   all;

        phase           gas;    // omit for single-phase
        Tmin            200;    // minimum temperature [K]
    }
    \endverbatim

See also
    Foam::fv::fixedTemperatureConstraint

\*---------------------------------------------------------------------------*/

#ifndef limitEnergyField_H
#define limitEnergyField_H

#include "cellSetOption.H"
#include "basicThermo.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                      Class limitEnergyField Declaration
\*---------------------------------------------------------------------------*/

class limitEnergyField
:
    public fv::cellSetOption
{
    // Private Data

        //- Phase name (empty for single-phase)
        word phaseName_;

        //- Minimum allowable temperature [K]
        scalar Tmin_;


    // Private Member Functions

        //- No copy construct
        limitEnergyField(const limitEnergyField&) = delete;

        //- No copy assignment
        void operator=(const limitEnergyField&) = delete;

        //- Registry name of the basicThermo object
        word thermoName() const;


public:

    //- Runtime type information
    TypeName("limitEnergyField");


    // Constructors

        //- Construct from components
        limitEnergyField
        (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
        );


    //- Destructor
    virtual ~limitEnergyField() = default;


    // Member Functions

        //- Constrain the energy equation: force he = he(p, Tmin) where T < Tmin
        virtual void constrain(fvMatrix<scalar>& eqn, const label fieldi);

        //- Read dictionary
        virtual bool read(const dictionary& dict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
