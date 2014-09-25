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

#include "anisotropicVorton.H"

namespace Foam
{
  
anisotropicVorton::StructureParameters::StructureParameters()
{
}

anisotropicVorton::StructureParameters::StructureParameters(const dictionary&)
{
}

void anisotropicVorton::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void anisotropicVorton::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void anisotropicVorton::StructureParameters::write(Ostream&) const
{
}

/*
scalar anisotropicVorton::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

anisotropicVorton::anisotropicVorton()
: turbulentStructure(),
  epsilon_(pTraits<vector>::zero)
{}

anisotropicVorton::anisotropicVorton
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(s)
{}

anisotropicVorton::anisotropicVorton(BoostRandomGen& r, const vector& loc, const vector& initialDelta, const vector& v, const symmTensor& L, scalar minL,
  label creaface,
      const symmTensor& R)
: turbulentStructure(r, loc, initialDelta, v, L, minL, creaface,
      R),
  epsilon_(pTraits<vector>::zero)
{
}


anisotropicVorton::anisotropicVorton(const anisotropicVorton& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_)
{}

vector anisotropicVorton::fluctuation(const StructureParameters& pa, const vector& x) const
{
    vector delta_x = x - location();

    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    vector e1=L1_/l1, e2=L2_/l2, e3=L3_/l3;
    if 
        (
            (mag(delta_x&e1)  < (l1 / 2.0)) &&
            (mag(delta_x&e2)  < (l2 / 2.0)) &&
            (mag(delta_x&e3)  < (l3 / 2.0))
        )
    {
      vector f=
           (1.0 - 2.0*mag(delta_x&e1)  / l1 )
          *(1.0 - 2.0*mag(delta_x&e2)  / l2 )
          *(1.0 - 2.0*mag(delta_x&e3)  / l3 )
          * pTraits<vector>::one;

      return cmptMultiply(epsilon_, f) / sqrt( 1./81. );
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<anisotropicVorton> anisotropicVorton::New(Istream& s)
{
    return autoPtr<anisotropicVorton>(new anisotropicVorton(s));
}


void anisotropicVorton::randomize(BoostRandomGen& rand)
{
  //epsilon_ = 2.0*(rand.vector01() - 0.5*vector::one);
  epsilon_.x() = 2.0*(rand() - 0.5);
  epsilon_.y() = 2.0*(rand() - 0.5);
  epsilon_.z() = 2.0*(rand() - 0.5);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

anisotropicVorton::~anisotropicVorton()
{}


autoPtr<anisotropicVorton> anisotropicVorton::clone() const
{
    return autoPtr<anisotropicVorton>
        (
            new anisotropicVorton(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void anisotropicVorton::operator=(const anisotropicVorton& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("anisotropicVorton::operator=(const anisotropicVorton&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    epsilon_=rhs.epsilon_;
}

bool anisotropicVorton::operator!=(const anisotropicVorton& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const anisotropicVorton& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, anisotropicVorton& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    vector eps(s);
    ht.epsilon_=eps;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
