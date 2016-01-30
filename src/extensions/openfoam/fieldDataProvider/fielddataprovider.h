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

#ifndef FOAM_DATAINTERPOLATOR_H
#define FOAM_DATAINTERPOLATOR_H

#include "fvCFD.H"

#include "base/linearalgebra.h"

#include <map>
#include <vector>
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/ptr_container/ptr_map.hpp"

#include "vectorspacebase.h"

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
};



template<class T>
class uniformField
: public FieldDataProvider<T>
{
  boost::ptr_vector<T> values_;
  
  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("uniform");
  
  uniformField(Istream& is);
  uniformField(const uniformField<T>& o);
  
  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};




template<class T>
class nonuniformField
: public FieldDataProvider<T>
{
  boost::ptr_vector<Field<T> > values_;
  
  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("nonuniform");
  
  nonuniformField(Istream& is);
  nonuniformField(const nonuniformField<T>& o);
  nonuniformField(const Field<T>& uf);
  
  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};






template<class T>
class linearProfile
: public FieldDataProvider<T>
{
//   point p0_;
//   vector ep_, ex_, ez_;
  VectorSpaceBase base_;
  Map<label> cols_;
  std::vector<fileName> filenames_;
  mutable boost::ptr_map<int, insight::Interpolator> values_;
  
  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("linearProfile");
  
  linearProfile(Istream& is);
  linearProfile(const linearProfile<T>& o);

  virtual void read(Istream& is);
  virtual void writeSup(Ostream& os) const;

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};




template<class T>
class radialProfile
: public FieldDataProvider<T>
{
//   point p0_;
//   vector ep_, ex_, ez_;
  CylCoordVectorSpaceBase base_;
  Map<label> cols_;
  std::vector<fileName> filenames_;
  mutable boost::ptr_map<int, insight::Interpolator> values_;
  
  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("radialProfile");
  
  radialProfile(Istream& is);
  radialProfile(const radialProfile<T>& o);

  virtual void read(Istream& is);
  virtual void writeSup(Ostream& os) const;

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};




template<class T>
class fittedProfile
: public FieldDataProvider<T>
{
  VectorSpaceBase base_;
  std::vector< std::vector<arma::mat> > coeffs_;
  
  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("fittedProfile");
  
  fittedProfile(Istream& is);
  fittedProfile(const fittedProfile<T>& o);

  virtual void read(Istream& is);
  virtual void writeSup(Ostream& os) const;

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};


}

#define makeFieldDataProvider(Type)                                           \
typedef FieldDataProvider<Type> Type##FieldDataProvider;                      \
    defineNamedTemplateTypeNameAndDebug(Type##FieldDataProvider, 0);          \
                                                                              \
    defineTemplateRunTimeSelectionTable                                       \
    (                                                                         \
        Type##FieldDataProvider,                                              \
        Istream                                                               \
    )


#define makeFieldDataProviderType(SS, Type)                                   \
typedef SS<Type> Type##SS;                                                    \
    defineNamedTemplateTypeNameAndDebug(Type##SS, 0);                         \
                                                                              \
    Type##FieldDataProvider::addIstreamConstructorToTable<SS<Type> >          \
        add##SS##Type##ConstructorToTable_;


#ifdef NoRepository
#   include "fielddataprovider.cpp"
#endif

#endif // FOAM_DATAINTERPOLATOR_H
