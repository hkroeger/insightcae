/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering 
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#ifndef decayingTurbulenceSpot_H
#define decayingTurbulenceSpot_H

#include "turbulentStructure.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class decayingTurbulenceSpot;

Ostream& operator<<(Ostream&, const decayingTurbulenceSpot&);
Istream& operator>>(Istream&, decayingTurbulenceSpot&);

/*---------------------------------------------------------------------------*\
                           Class decayingTurbulenceSpot Declaration
\*---------------------------------------------------------------------------*/

class decayingTurbulenceSpot
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
    decayingTurbulenceSpot();
    decayingTurbulenceSpot(Istream&);
    decayingTurbulenceSpot(BoostRandomGen&, const point& p, const vector& initialDelta, const vector& v,  const tensor& Leig,
  label creaface);
    decayingTurbulenceSpot(const decayingTurbulenceSpot& p);
    
    ~decayingTurbulenceSpot();
    

    vector fluctuation(const StructureParameters& pa, const vector&) const;
    
    // Check

    // Edit
    void randomize(BoostRandomGen&);

    void operator=(const decayingTurbulenceSpot&);
    bool operator!=(const decayingTurbulenceSpot&) const;

    // Required for storage in SLList
    static autoPtr<decayingTurbulenceSpot> New(Istream& s);
    autoPtr<decayingTurbulenceSpot> clone() const;

    // Friend Functions

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const decayingTurbulenceSpot&);
    friend Istream& operator>>(Istream&, decayingTurbulenceSpot&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
