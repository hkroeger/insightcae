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

#ifndef INSIGHT_CAD_FEATURE_H
#define INSIGHT_CAD_FEATURE_H

#include "base/factory.h"
#include "base/cppextensions.h"
#include "cadtypes.h"
#include "astbase.h"

#include <set>
#include <memory>

#ifndef Q_MOC_RUN
#include "boost/concept_check.hpp"
#include "boost/shared_ptr.hpp"
#endif

#include "base/linearalgebra.h"
#include "occinclude.h"

#ifndef WIN32
namespace boost
{
  
template <>
struct is_arithmetic<arma::mat> 
: public true_type
{
};

}
#endif

namespace insight 
{
namespace cad 
{
  
class Sketch;




class Filter
{

protected:
    ConstFeaturePtr model_;
    
public:
    Filter();
    virtual ~Filter();

    virtual void initialize(ConstFeaturePtr m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const =0;
    
    inline const Feature& model() const { return *model_; }

    virtual FilterPtr clone() const =0;

    FilterPtr operator&&(const Filter& f2);
    FilterPtr operator!();

};



inline FilterPtr new_clone(const Filter& f)
{
    return f.clone();
}





template<class T>
class QuantityComputer
{
public:
    typedef std::shared_ptr<QuantityComputer<T> > Ptr;

protected:
    ConstFeaturePtr model_;

public:

    QuantityComputer()
    {}

    virtual ~QuantityComputer()
    {}

    virtual void initialize(ConstFeaturePtr m)
    {
        model_=m;
    }

    virtual bool isValidForFeature(FeatureID) const
    {
      return true;
    }
    
    virtual T evaluate(FeatureID) =0;
    virtual QuantityComputer::Ptr clone() const =0;

    typename QuantityComputer<T>::Ptr operator+(const typename QuantityComputer<T>::Ptr& other) const;
    typename QuantityComputer<T>::Ptr operator+(const T& constant) const;

    FilterPtr operator==(const typename QuantityComputer<T>::Ptr& other) const;
    FilterPtr operator==(const T& constant) const
    {
        return FilterPtr();
    }
};



typedef QuantityComputer<double> scalarQuantityComputer;
typedef QuantityComputer<arma::mat> matQuantityComputer;
typedef std::shared_ptr<scalarQuantityComputer> scalarQuantityComputerPtr;
typedef std::shared_ptr<matQuantityComputer> matQuantityComputerPtr;




std::ostream& operator<<(std::ostream& os, const FeatureSet& fs);
std::ostream& operator<<(std::ostream& os, const FeatureSetData& fs);




class FeatureSet
: public DependencySource
{
// #warning Should be a shared_ptr! Otherwise problems with feature sets from temporarily created shapes.
  
  /**
   * feature which shall be queried
   */
  ConstFeaturePtr model_;
    
  EntityType shape_;
  
  FeatureSetData data_;

protected:
  FeatureSet(const FeatureSet&o, TreeCloneMap& tcm);

public:
#ifndef SWIG
  DEPENDS_NOINVALIDATE((model_));
#endif
  CLONEABLE(FeatureSet);

  FeatureSet(const FeatureSet& o);
  FeatureSet(ConstFeaturePtr m, EntityType shape);
  FeatureSet(ConstFeaturePtr m, EntityType shape, FeatureID id);
  FeatureSet(ConstFeaturePtr m, EntityType shape, const FeatureSetData& ids);
  FeatureSet(ConstFeaturePtr m, EntityType shape, const std::vector<FeatureID>& ids);

  virtual ~FeatureSet();

  ConstFeaturePtr model() const;
  EntityType shape() const;

  size_t size() const;
  
  void safe_union(const FeatureSet& o);
  void safe_union(ConstFeatureSetPtr o);
  
  virtual const FeatureSetData& data() const;
  void setData(const FeatureSetData& d);
  void add(const FeatureID& e);
  
  operator const FeatureSetData& () const;
  operator TopAbs_ShapeEnum () const;
  
  virtual FeatureSetPtr clone() const;
  
  void write() const;

  void operator=(const FeatureSet& fs);

  virtual size_t calcFeatureSetHash() const;
};


#ifndef SWIG
class ASTBasedFeatureSet
    : public ASTBase,
      public FeatureSet
{
public:
    using FeatureSet::FeatureSet;
};
#endif

class ProvidedFeatureSet
    : public ASTBasedFeatureSet
{
    ConstFeaturePtr model_;
    std::string label_;

    ProvidedFeatureSet(const ProvidedFeatureSet&o, TreeCloneMap& tcm);
    ProvidedFeatureSet(
        ConstFeaturePtr m, EntityType shape,
        const std::string& label );

public:
    CREATE_FUNCTION(ProvidedFeatureSet);
    CLONEABLE(ProvidedFeatureSet);
#ifndef SWIG
    DEPENDS((model_));
#endif

    size_t calcHash() const override;
    void build() override;

    FeatureSetPtr clone() const override;

    const FeatureSetData& data() const override;

    size_t calcFeatureSetHash() const override;
};




class DeferredFeatureSet
    : public ASTBasedFeatureSet
{
  /**
   * basis for subset query
   */
    ConstFeatureSetPtr baseSet_;

    std::string filterexpr_;
    FeatureSetParserArgList refs_;


    size_t calcHash() const override;
    void build() override;

    DeferredFeatureSet(const DeferredFeatureSet&o, TreeCloneMap& tcm);

  /**
   * query an entire feature
   */
    DeferredFeatureSet
    (
        ConstFeaturePtr  m,
        EntityType shape,
        const std::string& filterexpr = "",
        const FeatureSetParserArgList& refs = FeatureSetParserArgList()
    );

  /**
   * query based on the result of a previous query
   */
    DeferredFeatureSet
    (
        ConstFeatureSetPtr q,
        const std::string& filterexpr = "",
        const FeatureSetParserArgList& refs = FeatureSetParserArgList()
    );

public:
    CREATE_FUNCTION(DeferredFeatureSet);
    CLONEABLE(DeferredFeatureSet);
#ifndef SWIG
    DEPENDS((baseSet_, refs_));
#endif

    inline ConstFeatureSetPtr baseSet() const { return baseSet_; }

    const FeatureSetData& data() const override;

    size_t calcFeatureSetHash() const override;
    FeatureSetPtr clone() const override;
};



template<EntityType ET>
FeatureSetPtr makeFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    )
{
    return DeferredFeatureSet::create(
        feat, ET,
        expression, refs
        );
}



template<EntityType ET>
FeatureSetPtr makeFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    )
{
    return DeferredFeatureSet::create<ConstFeatureSetPtr>(
        feat,
        expression, refs
        );
}



FeatureSetPtr makeVertexFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeEdgeFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeFaceFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeSolidFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );



FeatureSetPtr makeVertexFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeEdgeFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeFaceFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

FeatureSetPtr makeSolidFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression = "",
    const FeatureSetParserArgList& refs = {}
    );

}
}

#endif
