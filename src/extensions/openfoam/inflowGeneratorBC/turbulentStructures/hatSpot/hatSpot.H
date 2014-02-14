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

#include "turbulentStructure.H"

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
: public turbulentStructure
{

public:

    class StructureParameters
    {
    public:
        StructureParameters();
        StructureParameters(const dictionary&);

	void autoMap
	(
	    const fvPatchFieldMapper&
	);

	//- Reverse map the given fvPatchField onto this fvPatchField
	void rmap
	(
	    const fvPatchField<vector>&,
	    const labelList&
	);
    
        void write(Ostream&) const;
	
	scalar calcInfluenceLength(const vector& L);
    };

protected:
    const StructureParameters& p_;
    vector epsilon_;

public:

    // Constructors
    hatSpot();
    hatSpot(Istream&);
    hatSpot(const point& p, const vector& v, const symmTensor& L);
    hatSpot(const hatSpot& p);
    
    ~hatSpot();
    

    vector fluctuation(const vector&) const;
    
    // Check

    // Edit
    void randomize(Random&);

    void operator=(const hatSpot&);
    bool operator!=(const hatSpot&) const;

    // Required for storage in SLList
    static autoPtr<hatSpot> New(Istream& s);
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
