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

#include "vtkSmartPointer.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkPointInterpolator.h"
#include "vtkPolyData.h"
#include "vtkPoints.h"
#include "vtkDoubleArray.h"
#include "vtkGaussianKernel.h"
#include "vtkPointData.h"
#include "vtkCellCenters.h"
#include "vtkPolyDataReader.h"
#include "vtkGenericDataObjectReader.h"
#include "vtkProbeFilter.h"
#include "vtkDelaunay2D.h"

#include "fvCFD.H"

#include "base/linearalgebra.h"

#include <map>
#include <vector>
#include "boost/ptr_container/ptr_vector.hpp"
#include "boost/ptr_container/ptr_map.hpp"

#include "vectorspacebase.h"

#include "uniof.h"

#include "vtkconversion.h"

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
            const FixedSizeFieldDataProvider& o )
        : fdp_(o.fdp_->clone()),
          pp_(o.pp_),
          lastUpdateTime_(o.lastUpdateTime_),
          value_( o.value_ )
    {}

    FixedSizeFieldDataProvider(
            const FieldDataProvider<T>& fp,
            const PointProvider& pp )
        : fdp_(fp.clone()),
          pp_(pp),
          lastUpdateTime_(-GREAT),
          value_( pp_.size(), pTraits<T>::zero )
    {}

    FixedSizeFieldDataProvider(
            Istream& is,
            const PointProvider& pp )
        : fdp_(FieldDataProvider<T>::New(is)),
          pp_(pp),
          lastUpdateTime_(-GREAT),
          value_( pp_.size(), pTraits<T>::zero )
    {}

    bool needsUpdate(scalar t) const
    {
        return lastUpdateTime_<t;
    }

    const Field<T>& operator()(scalar t) const
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

    const FieldDataProvider<T>& fieldDataProvider() const
    {
        return fdp_();
    }
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
  uniformField(const T& uv);

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

  //- Map (and resize as needed) from self given a mapping object
  void autoMap
  (
      const fvPatchFieldMapper&
  );


  //- Reverse map the given fvPatchField onto this fvPatchField
  void rmap
  (
      const FieldDataProvider<T>&,
      const labelList&
  );
};






template<class T>
class linearProfile
: public FieldDataProvider<T>
{
  VectorSpaceBase base_;
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
  CylCoordVectorSpaceBase base_;
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



template<class T>
class vtkField
: public FieldDataProvider<T>
{
public:
    typedef std::vector<int> ComponentMap;

    static const ComponentMap VTKSymmTensorMap, OpenFOAMSymmTensorMap;

private:
  std::vector<fileName> vtkFiles_;
  std::vector<string> fieldNames_;
  ComponentMap componentMap_;
  word componentOrderName_;

  void setComponentMap(const word& mapSelection = word());

  mutable std::map<int, vtkSmartPointer<vtkDataObject> > data_;
  mutable std::map<long int, Field<T> > cache_;

  virtual void appendInstant(Istream& is);
  virtual void writeInstant(int i, Ostream& os) const;

public:
  //- Runtime type information
  TypeName("vtkField");

  vtkField(Istream& is);
  vtkField(const vtkField<T>& o);

  virtual tmp<Field<T> > atInstant(int i, const pointField& target) const;
  virtual autoPtr<FieldDataProvider<T> > clone() const;
};

template<>
void vtkField<symmTensor>::setComponentMap(const word& orderType);

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
