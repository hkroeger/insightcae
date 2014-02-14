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
    hatSpot

Description

Author

SourceFiles
    hatSpotI.H
    hatSpot.C
    hatSpotIO.C

\*---------------------------------------------------------------------------*/

#ifndef hatSpot_H
#define hatSpot_H

#include "scalar.H"
#include "vector.H"
#include "Random.H"
#include "IOdictionary.H"
#include "Istream.H"
#include "Ostream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class hatSpot;

Ostream& operator<<(Ostream&, const hatSpot&);
Istream& operator>>(Istream&, hatSpot&);

/*---------------------------------------------------------------------------*\
                           Class hatSpot Declaration
\*---------------------------------------------------------------------------*/

class hatSpot
{

public:

    class StructureParameters
    {
    public:
        scalarField L_;    // integral length scale
        scalar Lspot_; // spot length

        StructureParameters();
        StructureParameters(const dictionary&);
        StructureParameters(scalar);

        void write(Ostream&) const;
    };

private:
    // Private data

    vector location_;
    vector epsilon_;
    scalar L_;

    // Private Member Functions

public:


    static scalar calcInfluenceLength(const Parameters& p);


    // Constructors
    hatSpot();

    hatSpot(Istream&);

    //- Construct from components
    hatSpot(const vector&);
    
    //- Construct as copy
    hatSpot(const hatSpot&);
    
    // Selectors

    //- Select null constructed
    static autoPtr<hatSpot> New(Istream&);


    // Destructor

    ~hatSpot();
    

    // Member Functions

    // Access
    inline void setLocation(const vector& x) { location_=x; }
    inline const vector& location() const { return location_; }

    vector fluctuation(const Parameters&, const vector&) const;
    
    // Check

    // Edit
    void randomize(Random&);
    void moveForward(vector);
    
    // Write


    // Member Operators

    void operator=(const hatSpot&);
    bool operator!=(const hatSpot&) const;

    autoPtr<hatSpot> clone() const;

    // Friend Functions

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const hatSpot&);
    friend Istream& operator>>(Istream&, hatSpot&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
