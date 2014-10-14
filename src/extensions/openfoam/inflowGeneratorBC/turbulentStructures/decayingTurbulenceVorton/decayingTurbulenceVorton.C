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

#include "decayingTurbulenceVorton.H"

namespace Foam
{
  
decayingTurbulenceVorton::StructureParameters::StructureParameters()
{
}

decayingTurbulenceVorton::StructureParameters::StructureParameters(const dictionary&)
{
}

void decayingTurbulenceVorton::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void decayingTurbulenceVorton::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void decayingTurbulenceVorton::StructureParameters::write(Ostream&) const
{
}

/*
scalar decayingTurbulenceVorton::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

decayingTurbulenceVorton::decayingTurbulenceVorton()
: turbulentStructure(),
  epsilon_(pTraits<vector>::zero)
{}

decayingTurbulenceVorton::decayingTurbulenceVorton
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(s)
{}

decayingTurbulenceVorton::decayingTurbulenceVorton(BoostRandomGen& r, const vector& loc, const vector& initialDelta, const vector& v,  const symmTensor& L, scalar minL,
  label creaface,
      const symmTensor& R)
: turbulentStructure(r, loc, initialDelta, v, L, minL, creaface, R),
  epsilon_(pTraits<vector>::zero)
{
}


decayingTurbulenceVorton::decayingTurbulenceVorton(const decayingTurbulenceVorton& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_)
{}

vector decayingTurbulenceVorton::fluctuation(const StructureParameters& pa, const vector& x) const
{
    vector delta_x = x - location();

    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    scalar L=(l1+l2+l3)/3.;
    
    vector e1=L1_/l1, e2=L2_/l2, e3=L3_/l3;
    double Xx=mag(delta_x&e1);
    double Yy=mag(delta_x&e2);
    double Zz=mag(delta_x&e3);
    
    if 
        (
            (Xx  < 2.*L) &&
            (Yy  < 2.*L) &&
            (Zz  < 2.*L)
        )
    {
      vector f=
         (1./L) * exp(-(M_PI/2.)*magSqr(delta_x)/(L*L)) * (delta_x^epsilon_);

      return LundScaled
      (
	f / sqrt( 1./(3.*M_PI) )
      );
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<decayingTurbulenceVorton> decayingTurbulenceVorton::New(Istream& s)
{
    return autoPtr<decayingTurbulenceVorton>(new decayingTurbulenceVorton(s));
}


void decayingTurbulenceVorton::randomize(BoostRandomGen& rand)
{
  //epsilon_ = 2.0*(rand.vector01() - 0.5*vector::one);
  epsilon_.x() = 2.0*(rand() - 0.5);
  epsilon_.y() = 2.0*(rand() - 0.5);
  epsilon_.z() = 2.0*(rand() - 0.5);
//   epsilon_ /= SMALL+mag(epsilon_);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

decayingTurbulenceVorton::~decayingTurbulenceVorton()
{}


autoPtr<decayingTurbulenceVorton> decayingTurbulenceVorton::clone() const
{
    return autoPtr<decayingTurbulenceVorton>
        (
            new decayingTurbulenceVorton(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void decayingTurbulenceVorton::operator=(const decayingTurbulenceVorton& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("decayingTurbulenceVorton::operator=(const decayingTurbulenceVorton&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    epsilon_=rhs.epsilon_;
}

bool decayingTurbulenceVorton::operator!=(const decayingTurbulenceVorton& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const decayingTurbulenceVorton& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, decayingTurbulenceVorton& ht)
{
    s >> *static_cast<turbulentStructure*>(&ht);
    s >> ht.epsilon_;
    return s;
}


// * * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


// ************************************************************************* //

}
