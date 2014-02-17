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

/**
 * base class for turbulent structures.
 * derived from point => represents its location
 */
class turbulentStructure
: public point
{
protected:
  /**
   * convection velocity of the structure
   */
  vector velocity_;
  
  /**
   * prescribed turbulent length scale, converted into (principal axes)*(principal values)
   */
  vector L1_, L2_, L3_;

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
    turbulentStructure(const point& p, const vector& v, const symmTensor& L);

    /**
     * copy constructor
     */
    turbulentStructure(const turbulentStructure& p);

    inline void setVelocity(const vector& x) { velocity_=x; }
    inline void setLocation(const point& x) { point::operator=(x); }
    inline const point& location() const { return *this; }
    
    // Edit
    void moveForward(scalar dt);
    
    friend Ostream& operator<<(Ostream&, const turbulentStructure&);
    friend Istream& operator>>(Istream&, turbulentStructure&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
