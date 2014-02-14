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

Class
    turbulentStructure

Description

Author

SourceFiles
    turbulentStructureI.H
    turbulentStructure.C
    turbulentStructureIO.C

\*---------------------------------------------------------------------------*/

#ifndef turbulentStructure_H
#define turbulentStructure_H

#include "fvCFD.H"
#include "Random.H"
#include "IOdictionary.H"
#include "Istream.H"
#include "Ostream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class turbulentStructure;

Ostream& operator<<(Ostream&, const turbulentStructure&);
Istream& operator>>(Istream&, turbulentStructure&);

class turbulentStructure
{
protected:
    point location_;

public:    
    /**
     * default
     */
    turbulentStructure();
    
    /**
     * Create from stream (during restart)
     */
    turbulentStructure(Istream&);
    
    /**
     * create at some location (during run)
     */
    turbulentStructure(const point& p);

    /**
     * copy constructor
     */
    turbulentStructure(const turbulentStructure& p);

    inline void setLocation(const vector& x) { location_=x; }
    inline const vector& location() const { return location_; }
    
    // Edit
    void moveForward(vector);
    
    friend Ostream& operator<<(Ostream&, const turbulentStructure&);
    friend Istream& operator>>(Istream&, turbulentStructure&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
