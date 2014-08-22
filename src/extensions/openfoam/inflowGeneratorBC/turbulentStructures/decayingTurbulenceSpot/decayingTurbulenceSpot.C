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

#include "decayingTurbulenceSpot.H"

namespace Foam
{
  
decayingTurbulenceSpot::StructureParameters::StructureParameters()
{
}

decayingTurbulenceSpot::StructureParameters::StructureParameters(const dictionary&)
{
}

void decayingTurbulenceSpot::StructureParameters::autoMap
(
    const fvPatchFieldMapper&
)
{
}

//- Reverse map the given fvPatchField onto this fvPatchField
void decayingTurbulenceSpot::StructureParameters::rmap
(
    const fvPatchField<vector>&,
    const labelList&
)
{
}

void decayingTurbulenceSpot::StructureParameters::write(Ostream&) const
{
}

/*
scalar decayingTurbulenceSpot::calcInfluenceLength(const Parameters& p)
{
#warning Please check the factor!
    return (4./3.)*p.L_;
}*/

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

decayingTurbulenceSpot::decayingTurbulenceSpot()
: turbulentStructure(),
  epsilon_(pTraits<vector>::zero)
{}

decayingTurbulenceSpot::decayingTurbulenceSpot
(
    Istream& s
)
: turbulentStructure(s),
  epsilon_(s)
{}

decayingTurbulenceSpot::decayingTurbulenceSpot(BoostRandomGen& r, const vector& loc, const vector& initialDelta, const vector& v,  const tensor& Leig,
  label creaface)
: turbulentStructure(r, loc, initialDelta, v, Leig, creaface),
  epsilon_(pTraits<vector>::zero)
{
}


decayingTurbulenceSpot::decayingTurbulenceSpot(const decayingTurbulenceSpot& o)
: turbulentStructure(o),
  epsilon_(o.epsilon_)
{}

vector decayingTurbulenceSpot::fluctuation(const StructureParameters& pa, const vector& x) const
{
    vector delta_x = x - location();

    scalar l1=mag(L1_), l2=mag(L2_), l3=mag(L3_);
    scalar l0Sq=sqr(l1);
    scalar l1Sq=sqr(l2);
    scalar l2Sq=sqr(l3);
    vector e1=L1_/l1, e2=L2_/l2, e3=L3_/l3;
    double Xx=mag(delta_x&e1);
    double Yy=mag(delta_x&e2);
    double Zz=mag(delta_x&e3);
    
    if 
        (
            (Xx  < l1) &&
            (Yy  < l2) &&
            (Zz  < l3)
        )
    {
      vector f=
           ( exp(-0.25*Xx*Xx*(2.+M_PI/l0Sq)-0.25*Yy*Yy*(2.+M_PI/l1Sq)-0.25*Zz*Zz*(2.+M_PI/l2Sq)) )
          *pow(M_PI, 9./2.)
          *sqrt(2.+M_PI/l0Sq)
	  *sqrt(2.+M_PI/l1Sq)
	  *sqrt(2.+M_PI/l2Sq)
	  *(1./8./l0Sq/l1Sq/l2Sq)
          * pTraits<vector>::one;

      return cmptMultiply(epsilon_, f) 
      / 
      sqrt( 2445.22*sqrt((M_PI+2.*l0Sq)*(M_PI+2.*l1Sq)*(M_PI+2.*l2Sq)) / pow(l0Sq*l1Sq*l2Sq,3) );
    }
  else
    return pTraits<vector>::zero;
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

autoPtr<decayingTurbulenceSpot> decayingTurbulenceSpot::New(Istream& s)
{
    return autoPtr<decayingTurbulenceSpot>(new decayingTurbulenceSpot(s));
}


void decayingTurbulenceSpot::randomize(BoostRandomGen& rand)
{
  //epsilon_ = 2.0*(rand.vector01() - 0.5*vector::one);
  epsilon_.x() = 2.0*(rand() - 0.5);
  epsilon_.y() = 2.0*(rand() - 0.5);
  epsilon_.z() = 2.0*(rand() - 0.5);
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

decayingTurbulenceSpot::~decayingTurbulenceSpot()
{}


autoPtr<decayingTurbulenceSpot> decayingTurbulenceSpot::clone() const
{
    return autoPtr<decayingTurbulenceSpot>
        (
            new decayingTurbulenceSpot(*this)
        );
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

void decayingTurbulenceSpot::operator=(const decayingTurbulenceSpot& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("decayingTurbulenceSpot::operator=(const decayingTurbulenceSpot&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    turbulentStructure::operator=(rhs);
    epsilon_=rhs.epsilon_;
}

bool decayingTurbulenceSpot::operator!=(const decayingTurbulenceSpot& o) const
{
    return 
        (location()!=o.location())
        ||
        (epsilon_!=o.epsilon_);
}

Ostream& operator<<(Ostream& s, const decayingTurbulenceSpot& ht)
{
    s << *static_cast<const turbulentStructure*>(&ht);
    s<<ht.epsilon_<<endl;
    return s;
}

Istream& operator>>(Istream& s, decayingTurbulenceSpot& ht)
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
