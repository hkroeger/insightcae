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

#ifndef FOAM_FIELDDATAPROVIDER_H
#define FOAM_FIELDDATAPROVIDER_H

#include <limits>


#include "fvCFD.H"

#include "base/linearalgebra.h"

#include <map>
#include <vector>
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/ptr_container/ptr_map.hpp"

#include "vectorspacebase.h"

#include "uniof.h"



namespace Foam 
{




template<class T>
class FieldDataProvider
: public refCount
{
protected:
  List<scalar> timeInstants_;
  
  virtual void appendInstant(Istream& is) =0;
  virtual void writeInstant(int i, Ostream& os) const =0;
  
public:
  //- Runtime type information
  TypeName("FieldDataProvider");

  //- Declare runtime constructor selection table
  declareRunTimeSelectionTable
  (
      autoPtr,
      FieldDataProvider,
      Istream,
      (
        Istream& is
      ),
      (is)
  );
  
  //- Selector
  static autoPtr<FieldDataProvider> New
  (
      Istream& is
  );

    
  FieldDataProvider();
  FieldDataProvider(const FieldDataProvider<T>& o);
  FieldDataProvider(Istream& is);
  
  //- Destructor
  virtual ~FieldDataProvider();

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const =0;
  tmp<Field<T> > operator()(double time, const pointField& target) const;
  
  virtual autoPtr<FieldDataProvider<T> > clone() const =0;
  
  virtual void read(Istream& is);
  virtual void write(Ostream& os) const;
  virtual void writeSup(Ostream& os) const;
  
  void writeEntry(const word& key, Ostream& os) const;

  //- Map (and resize as needed) from self given a mapping object
  virtual void autoMap
  (
      const fvPatchFieldMapper&
  );


  //- Reverse map the given fvPatchField onto this fvPatchField
  virtual void rmap
  (
      const FieldDataProvider<T>&,
      const labelList&
  );

};




template<class T, class PointProvider>
class FixedSizeFieldDataProvider
{
    autoPtr<FieldDataProvider<T> > fdp_;
    const PointProvider& pp_;
    scalar lastUpdateTime_;
    Field<T> value_;

public:
    typedef FieldDataProvider<T> input_type;

    FixedSizeFieldDataProvider(
        const FixedSizeFieldDataProvider& o );

    FixedSizeFieldDataProvider(
        const FieldDataProvider<T>& fp,
        const PointProvider& pp );

    FixedSizeFieldDataProvider(
        Istream& is,
        const PointProvider& pp );

    bool needsUpdate(scalar t) const;

    const Field<T>& operator()(scalar t) const;

    const FieldDataProvider<T>& fieldDataProvider() const;
};




template<class T, class CS>
class CylCoordProfile
    : public FieldDataProvider<T>
{
    CS base_;
    std::vector<fileName> filenames_;
    mutable boost::ptr_map<int, insight::Interpolator> values_;

    virtual void appendInstant(Istream& is);
    virtual void writeInstant(int i, Ostream& os) const;

public:
    CylCoordProfile(Istream& is);
    CylCoordProfile(const CylCoordProfile<T,CS>& o);

    virtual void read(Istream& is);
    virtual void writeSup(Ostream& os) const;

    virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
};




}



#ifdef NoRepository
#   include "fielddataprovider.cpp"
#endif

#endif // FOAM_DATAINTERPOLATOR_H
