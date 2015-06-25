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
 */

#include "fielddataprovider.h"
#include "addToRunTimeSelectionTable.H"
#include "transform.H"
#include "OFstream.H"

#include "base/linearalgebra.h"
#include "boost/foreach.hpp"

#include "boost/version.hpp"
#if ( BOOST_VERSION < 105100 )
#define BOOST_NO_SCOPED_ENUMS
#else
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif
#include "boost/filesystem.hpp"

using namespace boost;
using namespace insight;

namespace Foam
{

template<class T>
tmp<Field<T> > FieldDataProvider<T>::operator()(double time, const pointField& target) const
{
  tmp<Field<T> > res;
  if ( (timeInstants_[0]>=time) || (timeInstants_.size()==1) )
  {
    res = atInstant(0, target);
  }
  else
  {
    if ( timeInstants_[timeInstants_.size()-1]<=time)
    {
      res = atInstant(timeInstants_.size()-1, target);
    }
    else
    {
      int ip;
      for (ip=1; ip<timeInstants_.size(); ip++)
      {
	if (timeInstants_[ip]>=time) break;
      }
      scalar wi=time-timeInstants_[ip-1];
      scalar wip=timeInstants_[ip]-time;
      res = ( wip*atInstant(ip-1, target) + wi*atInstant(ip, target) ) / (wi+wip);
    }
  }
  if (debug>1)
  {
    boost::filesystem::path fn=boost::filesystem::unique_path("./fielddataprovider-%%%%%%%%%.txt");
    Info<<"Dumping fielddata into "<<fn.c_str()<<endl;
    OFstream f(fn.c_str());
    forAll(target,j)
    {
      f<<target[j].x()<<" "<<target[j].y()<<" "<<target[j].z();
      for (int k=0; k<pTraits<T>::nComponents; k++)
	f<<" "<<component(res()[j], k);
      f<<endl;
    }
  }
//   Info<<"RETURN="<<res()<<endl;
  return res;
}

template<class T>
autoPtr<FieldDataProvider<T> > FieldDataProvider<T>::New
(
    Istream& is
)
{
  word typekey;
  is >> typekey;
  
  typename IstreamConstructorTable::iterator cstrIter =
      IstreamConstructorTablePtr_->find(typekey);
  
  if (cstrIter == IstreamConstructorTablePtr_->end())
  {   
      FatalErrorIn("FieldDataProvider<T>::New(const word&, const dictionary&)")
	  << "Unknown FieldDataProvider type "
	  << typekey << " for FieldDataProvider "
	  << nl
	  << "Valid FieldDataProvider types are:" << nl
	  << IstreamConstructorTablePtr_->toc() << nl
	  << exit(FatalError);
  }

  autoPtr<FieldDataProvider<T> > res(cstrIter()(is));
  res->read(is);
  return res;
}
  
  
template<class T>
FieldDataProvider<T>::FieldDataProvider(const FieldDataProvider<T>& o)
: refCount(),
  timeInstants_(o.timeInstants_)
{
}

template<class T>
FieldDataProvider<T>::FieldDataProvider(Istream& is)
: refCount()
{
}

template<class T>
FieldDataProvider<T>::~FieldDataProvider()
{
}

template<class T>
void FieldDataProvider<T>::read(Istream& is)
{
  token nexttoken(is);
  
  word timekey="steady";
  if (!nexttoken.isWord())
    is.putBack(nexttoken); // assume "steady"
  else
    timekey=nexttoken.wordToken();
    
  if (timekey=="steady")
  {
    timeInstants_.setSize(1);
    timeInstants_[0]=0.0;
    appendInstant(is);
  }
  else if (timekey=="unsteady")
  {
    DynamicList<scalar> times;
    for (token t(is); t.good(); t=token(is))
    {
//       Info<<t<<endl;
      if (t.isNumber())
      {
	times.append( t.number() );
	appendInstant(is);
	if (is.eof()) break;
      }
      else
      {
	FatalErrorIn("FieldDataProvider<T>::FieldDataProvider(Istream& is)")
	  <<"expected time value, got "<<t<<"!"
	  <<abort(FatalError);
      }
    }
    times.shrink();
    if (times.size()<=0)
    {
      FatalErrorIn("FieldDataProvider<T>::FieldDataProvider(Istream& is)")
	<<"unsteady input specified but no time instants given!"
	<<abort(FatalError);
    }
    timeInstants_.transfer(times);
  }
  else
  {
    FatalErrorIn("FieldDataProvider<T>::FieldDataProvider(Istream& is)")
     << "unknown time series keyword: "<<timekey << endl
     << "choices: steady unsteady" << endl
     << abort(FatalError);
  }
  
//   Info<<timeInstants_<<endl;
}

template<class T>
void FieldDataProvider<T>::write(Ostream& os) const
{
  os << type() << token::SPACE;
  
  writeSup(os);
  
  os<<token::SPACE;
  
  if (timeInstants_.size()==1)
  {
    //os << "steady" << token::SPACE;
    writeInstant(0, os);
  }
  else
  {
    os << "unsteady";
  
    for (int i=0; i<timeInstants_.size(); i++)
    {
      os<<token::SPACE<<timeInstants_[i]<<token::SPACE;
      writeInstant(i, os);
    }
  }
}

template<class T>
void FieldDataProvider<T>::writeSup(Ostream& os) const
{
}

template<class T>
void FieldDataProvider<T>::writeEntry(const word& key, Ostream& os) const
{
  os.writeKeyword(key);
  write(os);
  os<<token::END_STATEMENT<<nl;
}




template<class T>  
uniformField<T>::uniformField(Istream& is)
: FieldDataProvider<T>(is)
{
}

template<class T>
void uniformField<T>::appendInstant(Istream& is)
{
  T v;
  is >> v;
  values_.push_back(new T(v));
}

template<class T>
void uniformField<T>::writeInstant(int i, Ostream& is) const
{
  is << values_[i];
}

template<class T>
tmp<Field<T> > uniformField<T>::atInstant(int i, const pointField& target) const
{
  tmp<Field<T> > res(new Field<T>(target.size()));
  res()=values_[i];
  return res;
}

template<class T>
uniformField<T>::uniformField(const uniformField<T>& o)
: FieldDataProvider<T>(o),
  values_(o.values_)
{
}
  
template<class T>
autoPtr<FieldDataProvider<T> > uniformField<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new uniformField<T>(*this));
}




template<class T>
void nonuniformField<T>::appendInstant(Istream& is)
{
  values_.push_back(new Field<T>(is));
}

template<class T>
void nonuniformField<T>::writeInstant(int i, Ostream& os) const
{
  values_[i].UList<T>::writeEntry(os);
}

  
template<class T>
nonuniformField<T>::nonuniformField(Istream& is)
: FieldDataProvider<T>(is)
{}
  
template<class T>
nonuniformField<T>::nonuniformField(const uniformField<T>& o)
: FieldDataProvider<T>(o),
  values_(o.values_)
{
}
  
template<class T>
tmp<Field<T> > nonuniformField<T>::atInstant(int i, const pointField& target) const
{
  tmp<Field<T> > res(new Field<T>(values_[i]));
  return res;
}

template<class T>
autoPtr<FieldDataProvider<T> > nonuniformField<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new nonuniformField<T>(*this));
}



template<class T>  
linearProfile<T>::linearProfile(Istream& is)
: FieldDataProvider<T>(is)
{
}

template<class T>
void linearProfile<T>::appendInstant(Istream& is)
{
  fileName fn;
  is >> fn;
  filenames_.push_back(fn);
  
//   arma::mat xy;
//   fn.expand();
//   xy.load(fn.c_str(), arma::raw_ascii);
//   values_.push_back(new insight::Interpolator(xy, true) );
}

template<class T>
void linearProfile<T>::writeInstant(int i, Ostream& is) const
{
  is << filenames_[i];
}

template<class T>
tmp<Field<T> > linearProfile<T>::atInstant(int idx, const pointField& target) const
{
  if (values_.find(idx)==values_.end())
  {
    fileName fn=filenames_[idx];
    arma::mat xy;
    fn.expand();
    xy.load(fn.c_str(), arma::raw_ascii);
    std::auto_ptr<insight::Interpolator> newipol(new insight::Interpolator(xy, true));
    values_.insert(idx, newipol);
  }
  
  tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
  Field<T>& res=resPtr();

//   vector ey = - (ex_ ^ ez_);
//   tensor tt(ex_, ey, ez_);
// //   Info<<ey<<tt<<endl;
  

  forAll(target, pi)
  {
    double t = base_.t(target[pi]);
    
    arma::mat q = (*values_.find(idx)->second)(t);
    
//     std::cout<<target[pi].x()<<" "<<target[pi].y()<<" "<<target[pi].z()<<" >> "<<t<<" >> "<<q<<std::endl;
    
//     Info<<cols_<<endl;
//     std::cout<<q<<std::endl;
    for (int c=0; c<q.n_elem; c++)
    {
      if (cols_.found(c)) //(cmap[c]>=0) // if column is used
      {
	setComponent( res[pi], cols_[c] ) = q(c);
      }
    }
    res[pi]=base_(res[pi]); //transform(tt, res[pi]);
  }
//    Info<<"res="<<res<<endl;
  return resPtr;
}

template<class T>
linearProfile<T>::linearProfile(const linearProfile<T>& o)
: FieldDataProvider<T>(o),
  base_(o.base_), //p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
  cols_(o.cols_),
  filenames_(o.filenames_),
  values_(o.values_)
{
}

template<class T>
void linearProfile<T>::read(Istream& is)
{
  base_.read(is);
  is >> cols_;
  FieldDataProvider<T>::read(is);
}
  
template<class T>
void linearProfile<T>::writeSup(Ostream& os) const
{
  base_.writeSup(os);
  os << token::SPACE << cols_;
}
  
template<class T>
autoPtr<FieldDataProvider<T> > linearProfile<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new linearProfile<T>(*this));
}




template<class T>  
radialProfile<T>::radialProfile(Istream& is)
: FieldDataProvider<T>(is)
{
}

template<class T>
void radialProfile<T>::appendInstant(Istream& is)
{
  fileName fn;
  is >> fn;
  filenames_.push_back(fn);
  
//   arma::mat xy;
//   fn.expand();
//   xy.load(fn.c_str(), arma::raw_ascii);
//   
//   values_.push_back(new insight::Interpolator(xy, true) );
}

template<class T>
void radialProfile<T>::writeInstant(int i, Ostream& is) const
{
  is << filenames_[i];
}

template<class T>
tmp<Field<T> > radialProfile<T>::atInstant(int idx, const pointField& target) const
{
  if (values_.find(idx)==values_.end())
  {
    fileName fn=filenames_[idx];
    arma::mat xy;
    fn.expand();
    xy.load(fn.c_str(), arma::raw_ascii);
    std::auto_ptr<insight::Interpolator> newipol(new insight::Interpolator(xy, true));
    values_.insert(idx, newipol);
  }

  tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
  Field<T>& res=resPtr();

//   vector ey = - (ex_ ^ ez_);
//   tensor tt(ex_, ey, ez_);
// //   Info<<ey<<tt<<endl;
  

  forAll(target, pi)
  {
    double t = base_.t(target[pi]);
    
    arma::mat q = (*values_.find(idx)->second)(t);
    
//     std::cout<<target[pi].x()<<" "<<target[pi].y()<<" "<<target[pi].z()<<" >> "<<t<<" >> "<<q<<std::endl;
    
//     Info<<cols_<<endl;
//     std::cout<<q<<std::endl;
    for (int c=0; c<q.n_elem; c++)
    {
      if (cols_.found(c)) //(cmap[c]>=0) // if column is used
      {
	setComponent( res[pi], cols_[c] ) = q(c);
      }
    }
    res[pi]=base_(res[pi], target[pi]); //transform(tt, res[pi]);
  }
//    Info<<"res="<<res<<endl;
  return resPtr;
}

template<class T>
radialProfile<T>::radialProfile(const radialProfile<T>& o)
: FieldDataProvider<T>(o),
  base_(o.base_), //p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
  cols_(o.cols_),
  filenames_(o.filenames_),
  values_(o.values_)
{
}

template<class T>
void radialProfile<T>::read(Istream& is)
{
  base_.read(is);
  is >> cols_;
  FieldDataProvider<T>::read(is);
}
  
template<class T>
void radialProfile<T>::writeSup(Ostream& os) const
{
  base_.writeSup(os);
  os << token::SPACE << cols_;
}
  
template<class T>
autoPtr<FieldDataProvider<T> > radialProfile<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new radialProfile<T>(*this));
}




template<class T>  
fittedProfile<T>::fittedProfile(Istream& is)
: FieldDataProvider<T>(is)
{
}

template<class T>
void fittedProfile<T>::appendInstant(Istream& is)
{
  std::vector<arma::mat> ccoeffs;
  for (int c=0; c<pTraits<T>::nComponents; c++)
  {
    token ct(is);
    if (ct.pToken()!=token::BEGIN_SQR)
      FatalErrorIn("appendInstant") << "Expected "<<token::BEGIN_SQR << abort(FatalError);
    std::vector<double> coeff;
    do
    {
      token nt(is);
      if (!nt.isNumber())
	FatalErrorIn("appendInstant") << "Expected number, got "<<nt<< abort(FatalError);
      coeff.push_back(nt.number());
      {
	token nt2(is);
	if (nt2.isPunctuation() && (nt2.pToken()==token::END_SQR))
	  break;
	else
	  is.putBack(nt2);
      }
    } while (!is.eof());
    
    ccoeffs.push_back(arma::mat(coeff.data(), coeff.size(), 1));
  }
  
  coeffs_.push_back(ccoeffs);
}

template<class T>
void fittedProfile<T>::writeInstant(int i, Ostream& is) const
{
  const std::vector<arma::mat>& ccoeffs=coeffs_[i];
  BOOST_FOREACH(const arma::mat& c, ccoeffs)
  {
    is << token::BEGIN_SQR << token::SPACE;
    for (unsigned int j=0; j<c.n_elem; j++)
      is << c(j) << token::SPACE;
    is << token::END_SQR << token::SPACE;
  }
}

template<class T>
tmp<Field<T> > fittedProfile<T>::atInstant(int idx, const pointField& target) const
{
  tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
  Field<T>& res=resPtr();  

  forAll(target, pi)
  {
    double t = base_.t(target[pi]);
    
    for (int c=0; c<pTraits<T>::nComponents; c++)
    {
      arma::mat coeff = coeffs_[idx][c];
      setComponent( res[pi], c )=evalPolynomial(t, coeff);
    }
    
    res[pi]=base_(res[pi]); //transform(tt, res[pi]);
  }
//   Info<<"res="<<res<<endl;
  return resPtr;
}

template<class T>
fittedProfile<T>::fittedProfile(const fittedProfile<T>& o)
: FieldDataProvider<T>(o),
  base_(o.base_), //p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
  coeffs_(o.coeffs_)
{
}

template<class T>
void fittedProfile<T>::read(Istream& is)
{
  base_.read(is);
  FieldDataProvider<T>::read(is);
}
  
template<class T>
void fittedProfile<T>::writeSup(Ostream& os) const
{
  base_.writeSup(os);
}
  
template<class T>
autoPtr<FieldDataProvider<T> > fittedProfile<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new fittedProfile<T>(*this));
}

}
