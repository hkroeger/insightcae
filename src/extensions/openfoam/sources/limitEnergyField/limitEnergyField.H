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
    Constrains the energy equation to enforce temperature bounds [Tmin, Tmax].

    Two complementary mechanisms are applied:

    1. constrain(fvMatrix<scalar>&): fires inside every energy equation
       solve (before the linear solve).  Replaces matrix rows where T < Tmin
       or T > Tmax with the Dirichlet constraint he = he(p, Tbound).
       This bounds the *result* of the solve.

    2. correct(volScalarField& he): fires after every energy solve and
       correctThermo call.  Clips both he and T directly so that T is never
       left at a stale negative (or extreme) value.  This is necessary because
       correctThermo uses the stored T as the initial guess T0 for the next
       Newton iteration; if T0 < 0 the solver crashes unconditionally, even
       when the new he is perfectly bounded.

    Using both hooks together prevents the stale-T0 crash that occurs when
    limitTemperature (which only clips he) is used alone.

Usage
    Example in system/fvOptions:
    \verbatim
    limitEnergyGas
    {
        type            limitEnergyField;
        active          yes;
        selectionMode   all;

        phase           gas;     // omit for single-phase
        Tmin            200;     // minimum temperature [K]
        Tmax            2000;    // maximum temperature [K] (optional)
    }
    \endverbatim

See also
    Foam::fv::fixedTemperatureConstraint
    Foam::fv::limitTemperature

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

        //- Maximum allowable temperature [K]
        scalar Tmax_;


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

        //- Constrain energy equation: force he = he(p, Tbound) where T is
        //  out of [Tmin, Tmax].  Fires before the linear solve.
        virtual void constrain(fvMatrix<scalar>& eqn, const label fieldi);

        //- Correct he AND T after the solve so T is never left stale/negative.
        //  Prevents the T0 < 0 crash in the next correctThermo call.
        virtual void correct(volScalarField& he);

        //- Read dictionary
        virtual bool read(const dictionary& dict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
