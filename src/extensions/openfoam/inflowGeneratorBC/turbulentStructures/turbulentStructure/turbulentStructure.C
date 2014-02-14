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
: location_(pTraits<point>::zero),
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

turbulentStructure::turbulentStructure(const point& p, const vector& v, const symmTensor& L)
: location_(p),
  velocity_(v)
{
  vector evals(eigenValues(L));
  L1_ = eigenVector(L, evals.x()) * evals.x();
  L2_ = eigenVector(L, evals.y()) * evals.y();
  L3_ = eigenVector(L, evals.z()) * evals.z();
  tensor dbg=eigenVectors(L);
  Info<<"Check: ("<<mag(L1_)<<" "<<mag(L2_)<<" "<<mag(L3_)<<") == "<<(dbg.T()*L*dbg)<<endl;
  
  p -= (v/mag(v)) * 0.5*max(evals); // start 1/2 of the max. length scale before inlet plane
}

turbulentStructure::turbulentStructure(const turbulentStructure& o)
: location_(o.location_),
  velocity_(o.velocity_),
  L1_(o.L1_),
  L2_(o.L2_),
  L3_(o.L3_)
{
}


void turbulentStructure::moveForward(scalar dt)
{
  location_+=dt*velocity_;
}

Ostream& operator<<(Ostream& s, const turbulentStructure& ht)
{
  s<<ht.location_<<endl;
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
    ht.location_=loc;
    ht.velocity_=v;
    ht.L1_=L1;
    ht.L2_=L2;
    ht.L3_=L3;
    return s;
}

}
