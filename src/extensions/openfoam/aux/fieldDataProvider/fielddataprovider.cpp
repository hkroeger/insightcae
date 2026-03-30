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
#include "IFstream.H"

#include "base/linearalgebra.h"
#include "base/vtktools.h"
#include "boost/foreach.hpp"

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
            {
                f<<" "<<component(res()[j], k);
            }
            f<<endl;
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
  
  auto cstrIter = IstreamConstructorTablePtr_->find(typekey);
  
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
FieldDataProvider<T>::FieldDataProvider()
{
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
    {
        is.putBack(nexttoken); // assume "steady"
    }
    else
    {
        timekey=nexttoken.wordToken();
    }
    
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
void FieldDataProvider<T>::autoMap
(
    const fvPatchFieldMapper&
)
{
}


//- Reverse map the given fvPatchField onto this fvPatchField
template<class T>
void FieldDataProvider<T>::rmap
(
    const FieldDataProvider<T>&,
    const labelList&
)
{
}









template<class T, class PointProvider>
FixedSizeFieldDataProvider<T,PointProvider>::
    FixedSizeFieldDataProvider(
    const FixedSizeFieldDataProvider& o )
    : fdp_(o.fdp_->clone()),
    pp_(o.pp_),
    lastUpdateTime_(o.lastUpdateTime_),
    value_( o.value_ )
{}

template<class T, class PointProvider>
FixedSizeFieldDataProvider<T,PointProvider>::
    FixedSizeFieldDataProvider(
    const FieldDataProvider<T>& fp,
    const PointProvider& pp )
    : fdp_(fp.clone()),
    pp_(pp),
    lastUpdateTime_(-GREAT),
    value_( pp_.size(), pTraits<T>::zero )
{}

template<class T, class PointProvider>
FixedSizeFieldDataProvider<T,PointProvider>::
    FixedSizeFieldDataProvider(
    Istream& is,
    const PointProvider& pp )
    : fdp_(FieldDataProvider<T>::New(is)),
    pp_(pp),
    lastUpdateTime_(-GREAT),
    value_( pp_.size(), pTraits<T>::zero )
{}

template<class T, class PointProvider>
bool
FixedSizeFieldDataProvider<T,PointProvider>::
    needsUpdate(scalar t) const
{
    return lastUpdateTime_<t;
}

template<class T, class PointProvider>
const Field<T>&
FixedSizeFieldDataProvider<T,PointProvider>::
    operator()(scalar t) const
{
    if ( needsUpdate(t) )
    {
        auto * nc = const_cast<FixedSizeFieldDataProvider*>(this);
        nc->lastUpdateTime_ = t;
        nc->value_ =
            fdp_()( t, this->pp_.faceCentres() );
    }
    return value_;
}

template<class T, class PointProvider>
const FieldDataProvider<T>&
FixedSizeFieldDataProvider<T,PointProvider>::
    fieldDataProvider() const
{
    return fdp_();
}











template<class T, class CS>
void CylCoordProfile<T,CS>::appendInstant(Istream& is)
{
    fileName fn;
    is >> fn;
    filenames_.push_back(fn);
}

template<class T, class CS>
void CylCoordProfile<T,CS>::writeInstant(int i, Ostream& is) const
{
    is << filenames_[i];
}

template<class T, class CS>
tmp<Field<T> > CylCoordProfile<T,CS>::atInstant(int idx, const pointField& target) const
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
    Field<T>& res = UNIOF_TMP_NONCONST(resPtr);


    forAll(target, pi)
    {
        double t = base_.t(target[pi]);
        arma::mat q = (*values_.find(idx)->second)(t);
        for (size_t c=0; c<q.n_elem; c++)
        {
            setComponent( res[pi], c ) = q(c);
        }
        res[pi]=base_(res[pi], target[pi]);
    }

    return resPtr;
}

template<class T, class CS>
CylCoordProfile<T,CS>::CylCoordProfile(Istream& is)
    : FieldDataProvider<T>(is)
{
}

template<class T, class CS>
CylCoordProfile<T,CS>::CylCoordProfile(const CylCoordProfile<T,CS>& o)
    : FieldDataProvider<T>(o),
    base_(o.base_),
    filenames_(o.filenames_),
    values_(o.values_)
{}

template<class T, class CS>
void CylCoordProfile<T,CS>::read(Istream& is)
{
    base_.read(is);
    FieldDataProvider<T>::read(is);
}

template<class T, class CS>
void CylCoordProfile<T,CS>::writeSup(Ostream& os) const
{
    base_.writeSup(os);
}




}
