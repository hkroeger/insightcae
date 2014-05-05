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
#include <armadillo>

namespace Foam
{

turbulentStructure::turbulentStructure()
: point(pTraits<point>::zero),
  velocity_(pTraits<point>::zero),
  L1_(pTraits<vector>::zero),
  L2_(pTraits<vector>::zero),
  L3_(pTraits<vector>::zero),
  startPoint_(pTraits<point>::zero)
{
}

turbulentStructure::turbulentStructure(Istream& is)
{
  is >> *this;
}

turbulentStructure::turbulentStructure(BoostRandomGen& r, const point& p, const vector& v, const symmTensor& L, scalar minL)
: velocity_(v)
{
  /*
  // OF eigensystem analysis is crap for simplest cases: equal eigenvalues result in zero eigenvectors
  vector evals(eigenValues(L));
  L1_ = eigenVector(L, evals.x()) * evals.x();
  L2_ = eigenVector(L, evals.y()) * evals.y();
  L3_ = eigenVector(L, evals.z()) * evals.z();
  Info<<"L="<<L<<", evals="<<evals<<", L1="<<L1_<<", L2="<<L2_<<", L3="<<L3_<<endl;
  */
  
  // use armadillo instead
  arma::mat mL;
  mL 
  << L.xx()<<L.xy()<<L.xz()<<arma::endr
  << L.xy()<<L.yy()<<L.yz()<<arma::endr
  << L.xz()<<L.yz()<<L.zz()<<arma::endr;
  
  arma::vec eigval;
  arma::mat eigvec;
  eig_sym(eigval, eigvec, mL);
  //std::cout<<eigval<<eigvec<<std::endl;
  L1_ = vector(eigvec.col(0)(0), eigvec.col(0)(1), eigvec.col(0)(2)) * Foam::max(minL, eigval(0));
  L2_ = vector(eigvec.col(1)(0), eigvec.col(1)(1), eigvec.col(1)(2)) * Foam::max(minL, eigval(1));
  L3_ = vector(eigvec.col(2)(0), eigvec.col(2)(1), eigvec.col(2)(2)) * Foam::max(minL, eigval(2));
  
  /*
  // start at least 1/2 of the max. length scale before inlet plane
  startPoint_ = p - ((v/mag(v)) * r()*eigval.max()); 
  point::operator=(startPoint_);
  */
  startPoint_ = p;
  point::operator=(startPoint_);
}

turbulentStructure::turbulentStructure(const turbulentStructure& o)
: point(o),
  velocity_(o.velocity_),
  L1_(o.L1_),
  L2_(o.L2_),
  L3_(o.L3_),
  startPoint_(o.startPoint_)
{
}

scalar turbulentStructure::Lalong(const vector& x) const
{
  vector e=x/mag(x);
  diagTensor Alpha(mag(L1_), mag(L2_), mag(L3_));
  tensor Q( L1_/Alpha.xx(), L2_/Alpha.yy(), L3_/Alpha.zz() );
  //Info<<"RES="<<(e & ((Q.T()&Alpha&Q) & e))<<endl;
  return e & (Q.T()&Alpha&Q) & e;
}

scalar turbulentStructure::travelledDistance() const
{
  return mag(*this - startPoint_);
}

scalar turbulentStructure::passedThrough() const
{
  //Info<<"dist="<<travelledDistance()<<endl<< Foam::max(mag(L1_), Foam::max(mag(L2_), mag(L3_))) <<endl;
  return travelledDistance() > 1.5*Foam::max(mag(L1_), Foam::max(mag(L2_), mag(L3_)));
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
    startPoint_=rhs.startPoint_;
}

Ostream& operator<<(Ostream& s, const turbulentStructure& ht)
{
  s<<static_cast<const point&>(ht)<<endl;
  s<<ht.velocity_<<endl;
  s<<ht.L1_<<endl;
  s<<ht.L2_<<endl;
  s<<ht.L3_<<endl;
  s<<ht.startPoint_<<endl;
  return s;
}

Istream& operator>>(Istream& s, turbulentStructure& ht)
{
    vector loc(s);
    vector v(s);
    vector L1(s);
    vector L2(s);
    vector L3(s);
    point sp(s);
    ht.setLocation(loc);
    ht.velocity_=v;
    ht.L1_=L1;
    ht.L2_=L2;
    ht.L3_=L3;
    ht.startPoint_=sp;
    return s;
}

}
