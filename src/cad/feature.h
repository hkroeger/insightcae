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

#include <set>
#include <memory>
#include "boost/concept_check.hpp"
#include "boost/shared_ptr.hpp"

#include "base/linearalgebra.h"
#include "occinclude.h"

namespace boost
{
  
template <>
struct is_arithmetic<arma::mat> 
: public true_type
{
};

}

namespace insight 
{
namespace cad 
{
  
enum EntityType { Vertex, Edge, Face};


class SolidModel;

typedef int FeatureID;

class Filter
{

protected:
    const SolidModel* model_;
public:
    Filter();
    virtual ~Filter();

    virtual void initialize(const SolidModel& m);
    virtual void firstPass(FeatureID feature);
    virtual bool checkMatch(FeatureID feature) const =0;

    virtual boost::shared_ptr<Filter> clone() const =0;

    boost::shared_ptr<Filter> operator&&(const Filter& f2);
    boost::shared_ptr<Filter> operator!();

};

typedef boost::shared_ptr<Filter> FilterPtr;

class FeatureSet;

std::ostream& operator<<(std::ostream& os, const FeatureSet& fs);

class FeatureSet
: public std::set<FeatureID>
{
  const SolidModel& model_;
  EntityType shape_;
  
public:
  FeatureSet(const FeatureSet& o);
  FeatureSet(const SolidModel& m, EntityType shape);
  
  void safe_union(const FeatureSet& o);
  
  operator TopAbs_ShapeEnum () const;

  inline const SolidModel& model() const { return model_; }
  
  FeatureSet query(const FilterPtr& f) const;
  FeatureSet query(const std::string& queryexpr) const;
  
  std::auto_ptr<FeatureSet> clone() const;
  
  void write() const;
};

typedef boost::shared_ptr<FeatureSet> FeatureSetPtr;

}
}

#endif