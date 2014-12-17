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

namespace Foam 
{




template<class T>
class FieldDataProvider
: public refCount
{
protected:
  List<scalar> timeInstants_;
  
  virtual void appendInstant(Istream& is) =0;
  virtual void finishAppendInstances();
  
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

    
  FieldDataProvider(Istream& is);
  void read(Istream& is);
  
  //- Destructor
  virtual ~FieldDataProvider();

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const =0;
  tmp<Field<T> > operator()(double time, const pointField& target) const;
};



template<class T>
class uniformField
: public FieldDataProvider<T>
{
  DynamicList<T> values_;
  
  virtual void appendInstant(Istream& is);
  virtual void finishAppendInstances();

public:
  //- Runtime type information
  TypeName("uniformField");
  uniformField(Istream& is);
  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
};




}

#define makeFieldDataProvider(Type)                                           \
                                                                              \
    defineNamedTemplateTypeNameAndDebug(FieldDataProvider<Type>, 0);          \
                                                                              \
    defineTemplateRunTimeSelectionTable                                       \
    (                                                                         \
        FieldDataProvider<Type>,                                              \
        Istream                                                            \
    );


#define makeFieldDataProviderType(SS, Type)                                   \
                                                                              \
    defineNamedTemplateTypeNameAndDebug(SS<Type>, 0);                         \
                                                                              \
    FieldDataProvider<Type>::addIstreamConstructorToTable<SS<Type> >       \
        add##SS##Type##ConstructorToTable_;


#ifdef NoRepository
#   include "fielddataprovider.cpp"
#endif

#endif // FOAM_DATAINTERPOLATOR_H
