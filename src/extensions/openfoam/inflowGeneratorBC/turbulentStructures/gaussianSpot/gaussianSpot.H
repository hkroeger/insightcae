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
    gaussianSpot

Description

Author

SourceFiles
    gaussianSpotI.H
    gaussianSpot.C
    gaussianSpotIO.C

\*---------------------------------------------------------------------------*/

#ifndef gaussianSpot_H
#define gaussianSpot_H

#include "turbulentStructure.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class gaussianSpot;

Ostream& operator<<(Ostream&, const gaussianSpot&);
Istream& operator>>(Istream&, gaussianSpot&);

/*---------------------------------------------------------------------------*\
                           Class gaussianSpot Declaration
\*---------------------------------------------------------------------------*/

class gaussianSpot
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
    vector epsilon_;

public:

    // Constructors
    gaussianSpot();
    gaussianSpot(Istream&);
    gaussianSpot(BoostRandomGen&, const point& p, const vector& initialDelta, const vector& v,  const tensor& Leig,
  label creaface);
    gaussianSpot(const gaussianSpot& p);
    
    ~gaussianSpot();
    

    vector fluctuation(const StructureParameters& pa, const vector&) const;
    
    // Check

    // Edit
    void randomize(BoostRandomGen&);

    void operator=(const gaussianSpot&);
    bool operator!=(const gaussianSpot&) const;

    // Required for storage in SLList
    static autoPtr<gaussianSpot> New(Istream& s);
    autoPtr<gaussianSpot> clone() const;

    // Friend Functions

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const gaussianSpot&);
    friend Istream& operator>>(Istream&, gaussianSpot&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
