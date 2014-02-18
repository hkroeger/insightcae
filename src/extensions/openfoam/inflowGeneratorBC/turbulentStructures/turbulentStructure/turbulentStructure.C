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
    turbulentStructure

Description

Author

\*----------------------------------------------------------------------------*/

#include "turbulentStructure.H"

namespace Foam
{

turbulentStructure::turbulentStructure()
: point(pTraits<point>::zero),
  velocity_(pTraits<point>::zero),
  L1_(pTraits<vector>::zero),
  L2_(pTraits<vector>::zero),
  L3_(pTraits<vector>::zero)
{
}

turbulentStructure::turbulentStructure(Istream& is)
{
  is >> *this;
}

turbulentStructure::turbulentStructure(Random& r, const point& p, const vector& v, const symmTensor& L)
: point(p),
  velocity_(v)
{
  vector evals(eigenValues(L));
  L1_ = eigenVector(L, evals.x()) * sqrt(1./evals.x());
  L2_ = eigenVector(L, evals.y()) * sqrt(1./evals.y());
  L3_ = eigenVector(L, evals.z()) * sqrt(1./evals.z());
  
  // start at least 1/2 of the max. length scale before inlet plane
  *this -= (v/mag(v)) * (0.25*r.scalar01()+0.5)*Foam::max(Foam::max(evals.x(), evals.y()), evals.z()); 
}

turbulentStructure::turbulentStructure(const turbulentStructure& o)
: point(o),
  velocity_(o.velocity_),
  L1_(o.L1_),
  L2_(o.L2_),
  L3_(o.L3_)
{
}

scalar turbulentStructure::Lalong(const vector& x) const
{
  vector e=x/mag(x);
  diagTensor Alpha(1./magSqr(L1_), 1./magSqr(L2_), 1./magSqr(L3_));
  tensor Q( L1_/Alpha.xx(), L2_/Alpha.yy(), L3_/Alpha.zz());
  //Info<<"RES="<<(e & ((Q.T()&Alpha&Q) & e))<<endl;
  return ::sqrt(1./(e & (Q.T()&Alpha&Q) & e));
}


void turbulentStructure::moveForward(scalar dt)
{
  point::operator+=(dt*velocity_);
}

void turbulentStructure::operator=(const turbulentStructure& rhs)
{
    // Check for assignment to self
    if (this == &rhs)
    {
        FatalErrorIn("turbulentStructure::operator=(const turbulentStructure&)")
            << "Attempted assignment to self"
            << abort(FatalError);
    }

    point::operator=(rhs);
    velocity_=rhs.velocity_;
    L1_=rhs.L1_;
    L2_=rhs.L2_;
    L3_=rhs.L3_;
}

Ostream& operator<<(Ostream& s, const turbulentStructure& ht)
{
  s<<static_cast<const point&>(ht)<<endl;
  s<<ht.velocity_<<endl;
  s<<ht.L1_<<endl;
  s<<ht.L2_<<endl;
  s<<ht.L3_<<endl;
  return s;
}

Istream& operator>>(Istream& s, turbulentStructure& ht)
{
    vector loc(s);
    vector v(s);
    vector L1(s);
    vector L2(s);
    vector L3(s);
    ht.setLocation(loc);
    ht.velocity_=v;
    ht.L1_=L1;
    ht.L2_=L2;
    ht.L3_=L3;
    return s;
}

}
