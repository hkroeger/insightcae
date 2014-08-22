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

#ifndef homogeneousTurbulenceVorton_H
#define homogeneousTurbulenceVorton_H

#include "scalar.H"
#include "vector.H"
#include "boostRandomGen.H"
#include "IOdictionary.H"
#include "Istream.H"
#include "Ostream.H"
#include "turbulentStructure.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

class homogeneousTurbulenceVorton;

Ostream& operator<<(Ostream&, const homogeneousTurbulenceVorton&);
Istream& operator>>(Istream&, homogeneousTurbulenceVorton&);

/*---------------------------------------------------------------------------*\
                           Class homogeneousTurbulenceVorton Declaration
\*---------------------------------------------------------------------------*/

class homogeneousTurbulenceVorton
: public turbulentStructure
{

public:

    class StructureParameters
    {
    public:
        scalar eta_;  // Kolmogorov length
        scalar Cl_;
        scalar Ceta_;

        scalar C_2;
        scalar C_3;

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

private:
    vector omegav_;

public:

    // Constructors
    homogeneousTurbulenceVorton();
    homogeneousTurbulenceVorton(Istream&);
    homogeneousTurbulenceVorton(BoostRandomGen&, const point& p, const vector& initialDelta, const vector& v,  const tensor& Leig,
  label creaface);
    homogeneousTurbulenceVorton(const homogeneousTurbulenceVorton&);

    ~homogeneousTurbulenceVorton();

    vector fluctuation(const StructureParameters& pa, const vector&) const;

    void randomize(BoostRandomGen&);
    void operator=(const homogeneousTurbulenceVorton&);
    bool operator!=(const homogeneousTurbulenceVorton&) const;

    static autoPtr<homogeneousTurbulenceVorton> New(Istream&);
    autoPtr<homogeneousTurbulenceVorton> clone() const;

    // Friend Operators
    friend Ostream& operator<<(Ostream&, const homogeneousTurbulenceVorton&);
    friend Istream& operator>>(Istream&, homogeneousTurbulenceVorton&);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
