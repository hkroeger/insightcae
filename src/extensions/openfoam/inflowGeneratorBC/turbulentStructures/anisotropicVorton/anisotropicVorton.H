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

#ifndef anisotropicVorton_H
#define anisotropicVorton_H

#include "turbulentStructure.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class anisotropicVorton;

Ostream& operator<<(Ostream&, const anisotropicVorton&);
Istream& operator>>(Istream&, anisotropicVorton&);

/*---------------------------------------------------------------------------*\
                           Class anisotropicVorton Declaration
\*---------------------------------------------------------------------------*/

class anisotropicVorton
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
    scalar epsilon_;
    scalar rx_, ry_, rz_;
    scalar sx_, sy_, sz_;
    scalar k0_, C1_;

public:

    // Constructors
    anisotropicVorton();
    anisotropicVorton(Istream&);
    anisotropicVorton
    (
      BoostRandomGen&, 
      const point& p, 
      const vector& initialDelta, 
      const vector& v, 
      const symmTensor& L, 
      scalar minL,
      label creaface,
      const symmTensor& R
    );
    
    anisotropicVorton(const anisotropicVorton& p);
    
    ~anisotropicVorton();
    

    vector fluctuation(const StructureParameters& pa, const vector&) const;
    
    // Check

    // Edit
    void randomize(BoostRandomGen&);

    void operator=(const anisotropicVorton&);
    bool operator!=(const anisotropicVorton&) const;

    // Required for storage in SLList
    static autoPtr<anisotropicVorton> New(Istream& s);
    autoPtr<anisotropicVorton> clone() const;

    // Friend Functions

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const anisotropicVorton&);
    friend Istream& operator>>(Istream&, anisotropicVorton&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
