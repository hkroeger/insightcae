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

namespace Foam
{

template<class T>
tmp<Field<T> > FieldDataProvider<T>::operator()(double time, const pointField& target) const
{
  tmp<Field<T> > res;
  if ( (timeInstants_[0]>=time) || (timeInstants_.size()==1) )
  {
    res=atInstant(0, target);
  }
  else
  {
    if ( timeInstants_[timeInstants_.size()-1]<=time)
    {
//       Info<<timeInstants_<<endl;
      res=atInstant(timeInstants_.size()-1, target);
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
//       Info<<wi<<" "<<wip<<endl;
      res=( wip*atInstant(ip-1, target) + wi*atInstant(ip, target) ) / (wi+wip);
    }
  }
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
    os << "steady" << token::SPACE;
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
  tmp<Field<T> > res(new Field<T>(target.size(), values_[i]));
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
  
  arma::mat xy;
  fn.expand();
  xy.load(fn.c_str(), arma::raw_ascii);
  
  values_.push_back(new insight::Interpolator(xy, true) );
}

template<class T>
void linearProfile<T>::writeInstant(int i, Ostream& is) const
{
  is << filenames_[i];
}

template<class T>
tmp<Field<T> > linearProfile<T>::atInstant(int idx, const pointField& target) const
{
  tmp<Field<T> > resPtr(new Field<T>(target.size(), pTraits<T>::zero));
  Field<T>& res=resPtr();

  vector ey = - (ex_ ^ ez_);
  
  tensor tt(ex_, ey, ez_);
//   Info<<ey<<tt<<endl;
  
//   labelList cmap(pTraits<T>::nComponents, -1);
//   for (Map<word>::const_iterator i=cols_.begin(); i!=cols_.end(); i++)
//   {
//     for (int tc=0; tc<pTraits<T>::nComponents; tc++)
//     {
//       if (pTraits<T>::componentNames[tc]==i())
//       {
// 	cmap[i.key()]=tc;
// 	break;
//       }
//     }
//   }
//   Info<<"cmap="<<cmap<<endl;

  forAll(target, pi)
  {
    const point& p=target[pi];
    double t = (p-p0_)&ep_;
    
    arma::mat q = values_[idx](t);
//     std::cout<<"q="<<q<<std::endl;
    
    for (int c=0; c<q.n_elem; c++)
    {
//       std::cout<<c<<" "<<cmap[c]<<" "<<q(cmap[c])<<std::endl;
      if (cols_.found(c)) //(cmap[c]>=0) // if column is used
      {
	setComponent( res[pi], cols_[c] ) = q(c);
      }
    }
    res[pi]=transform(tt, res[pi]);
  }
  Info<<"res="<<res<<endl;
  return resPtr;
}

template<class T>
linearProfile<T>::linearProfile(const linearProfile<T>& o)
: FieldDataProvider<T>(o),
  p0_(o.p0_), ep_(o.ep_), ex_(o.ex_), ez_(o.ez_),
  cols_(o.cols_),
  filenames_(o.filenames_),
  values_(o.values_)
{
}

template<class T>
void linearProfile<T>::read(Istream& is)
{
  is >> p0_ >> ep_ >> cols_ >> ex_ >> ez_;
  FieldDataProvider<T>::read(is);
}
  
template<class T>
void linearProfile<T>::writeSup(Ostream& os) const
{
  os << p0_  << token::SPACE << ep_ << token::SPACE << cols_ << token::SPACE << ex_ <<token::SPACE << ez_;
}
  
template<class T>
autoPtr<FieldDataProvider<T> > linearProfile<T>::clone() const
{
  return autoPtr<FieldDataProvider<T> >(new linearProfile<T>(*this));
}


}
