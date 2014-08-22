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


#include "turbulentStructure.H"
#include <armadillo>
#include "inflowGeneratorBaseFvPatchVectorField.H"

namespace Foam
{

  
tensor ESAnalyze::eigenSystem(const symmTensor& L)
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
  
  return tensor
  (
   vector(eigvec.col(0)(0), eigvec.col(0)(1), eigvec.col(0)(2)) * eigval(0),
   vector(eigvec.col(1)(0), eigvec.col(1)(1), eigvec.col(1)(2)) * eigval(1),
   vector(eigvec.col(2)(0), eigvec.col(2)(1), eigvec.col(2)(2)) * eigval(2)
  );
}

ESAnalyze::ESAnalyze(const symmTensor& t)
: es_(eigenSystem(t))
{
}


bool ESAnalyze::clip(scalar minL)
{
  vector L1=c1();
  vector L2=c2();
  vector L3=c3();

  scalar mL1=mag(L1);
  scalar mL2=mag(L2);
  scalar mL3=mag(L3);
  
  bool clipped=false;
  
  if (mL1<minL)
  {
    L1*=minL/mL1;
    clipped=true;
  }
  if (mL2<minL)
  {
    L2*=minL/mL2;
    clipped=true;
  }
  if (mL3<minL)
  {
    L3*=minL/mL3;
    clipped=true;
  }
  
  es_=tensor(L1, L2, L3);
  
  return clipped;
}

scalar ESAnalyze::Lalong(const vector& x) const
{
  vector L1=c1();
  vector L2=c2();
  vector L3=c3();

  vector e=x/mag(x);
  diagTensor Alpha(mag(L1), mag(L2), mag(L3));
  tensor Q( L1/Alpha.xx(), L2/Alpha.yy(), L3/Alpha.zz() );
  //Info<<"RES="<<(e & ((Q.T()&Alpha&Q) & e))<<endl;
  return e & (Q.T()&Alpha&Q) & e;
}


turbulentStructure::turbulentStructure()
: point(pTraits<point>::zero),
  velocity_(pTraits<point>::zero),
  L1_(pTraits<vector>::zero),
  L2_(pTraits<vector>::zero),
  L3_(pTraits<vector>::zero),
  startPoint_(pTraits<point>::zero),
  creaFace_(-1)
{
}

turbulentStructure::turbulentStructure(Istream& is)
: creaFace_(-1)
{
  is >> *this;
}



turbulentStructure::turbulentStructure
(
  BoostRandomGen& r, 
  const point& p, 
  const vector& initialDelta, 
  const vector& v, 
  const tensor& Leig,
  label creaface
)
: velocity_(v),
  creaFace_(creaface)
{  
  L1_=Leig.x();
  L2_=Leig.y();
  L3_=Leig.z();

  initialPositioning(p, initialDelta);
}

turbulentStructure::turbulentStructure(const turbulentStructure& o)
: point(o),
  velocity_(o.velocity_),
  L1_(o.L1_),
  L2_(o.L2_),
  L3_(o.L3_),
  startPoint_(o.startPoint_),
  creaFace_(o.creaFace_)
{
}

// label turbulentStructure::nearestFace(const inflowGeneratorBaseFvPatchVectorField& patch) const
// {
//   if (nearestFace_[Pstream::myProcNo()]<0)
//   {
//     label nearestFace = patch.getNearestFace(footPoint_);
//     nearestFace_[Pstream::myProcNo()] = nearestFace;
//   }
//   
//   return nearestFace_[Pstream::myProcNo()];
// }


scalar turbulentStructure::travelledDistance() const
{
  return mag(*this - startPoint_);
}

scalar turbulentStructure::passedThrough() const
{
  //Info<<"dist="<<travelledDistance()<<endl<< Foam::max(mag(L1_), Foam::max(mag(L2_), mag(L3_))) <<endl;
  return travelledDistance() > Foam::max(mag(L1_), Foam::max(mag(L2_), mag(L3_)));
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
  footPoint_=rhs.footPoint_;
  creaFace_=rhs.creaFace_;
  
}

Ostream& operator<<(Ostream& s, const turbulentStructure& ht)
{
  s<<static_cast<const point&>(ht)<<endl;
  s<<ht.velocity_<<endl;
  s<<ht.L1_<<endl;
  s<<ht.L2_<<endl;
  s<<ht.L3_<<endl;
  s<<ht.startPoint_<<endl;
  s<<ht.footPoint_<<endl;
  s<<ht.creaFace_<<endl;
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
    point fp(s);
    label nf=readLabel(s);
    
    ht.setLocation(loc);
    ht.velocity_=v;
    ht.L1_=L1;
    ht.L2_=L2;
    ht.L3_=L3;
    ht.startPoint_=sp;
    ht.footPoint_=fp;
    ht.creaFace_=nf;
    
    return s;
}

}
