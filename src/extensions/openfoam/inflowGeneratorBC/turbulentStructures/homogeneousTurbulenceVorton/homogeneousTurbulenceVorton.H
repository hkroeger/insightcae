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
    homogeneousTurbulence

Description

Author

SourceFiles
    homogeneousTurbulenceI.H
    homogeneousTurbulence.C
    homogeneousTurbulenceIO.C

\*---------------------------------------------------------------------------*/

#ifndef homogeneousTurbulence_H
#define homogeneousTurbulence_H

#include "scalar.H"
#include "vector.H"
#include "Random.H"
#include "IOdictionary.H"
#include "Istream.H"
#include "Ostream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class homogeneousTurbulence;

Ostream& operator<<(Ostream&, const homogeneousTurbulence&);
Istream& operator>>(Istream&, homogeneousTurbulence&);

/*---------------------------------------------------------------------------*\
                           Class homogeneousTurbulence Declaration
\*---------------------------------------------------------------------------*/

class homogeneousTurbulence
{

public:

    class Parameters
    {
    public:
        scalar L_;    // integral length scale
        scalar eta_;  // Kolmogorov length
        scalar Cl_;
        scalar Ceta_;

        scalar C_2;
        scalar C_3;

        Parameters();
        Parameters(const dictionary&);
        Parameters(scalar, scalar, scalar, scalar);

        void write(Ostream&) const;
    };

private:
    // Private data

    vector location_;
    vector omegav_;

    // Private Member Functions

public:


    static scalar calcInfluenceLength(const Parameters& p);


    // Constructors
    homogeneousTurbulence();

    homogeneousTurbulence(Istream&);

    //- Construct from components
    homogeneousTurbulence(const vector&);
    
    //- Construct as copy
    homogeneousTurbulence(const homogeneousTurbulence&);
    
    // Selectors

    //- Select null constructed
    static autoPtr<homogeneousTurbulence> New(Istream&);


    // Destructor

    ~homogeneousTurbulence();
    

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

    void operator=(const homogeneousTurbulence&);
    bool operator!=(const homogeneousTurbulence&) const;

    autoPtr<homogeneousTurbulence> clone() const;

    // Friend Functions

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const homogeneousTurbulence&);
    friend Istream& operator>>(Istream&, homogeneousTurbulence&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
