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

#include "cadfeature.h"
#include "feature.h"
#include "boost/lexical_cast.hpp"
#include "boost/foreach.hpp"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


Filter::Filter()
: model_(NULL)
{
}

Filter::~Filter()
{
}

void Filter::initialize(ConstFeaturePtr m)
{
  model_=m;
}

void Filter::firstPass(FeatureID feature)
{
}



FeatureSet::FeatureSet(const FeatureSet& o)
: model_(o.model_),
  shape_(o.shape_)
{
  data_.insert( o.data().begin(), o.data().end() );
}

  
FeatureSet::FeatureSet(ConstFeaturePtr m, EntityType shape)
: model_(m),
  shape_(shape)
{
}

FeatureSet::FeatureSet(ConstFeaturePtr m, EntityType shape, FeatureID id)
: model_(m),
  shape_(shape)
{
  add(id);
}

FeatureSet::FeatureSet(ConstFeaturePtr m, EntityType shape, const FeatureSetData &ids)
: model_(m),
  shape_(shape),
  data_(ids)
{}

FeatureSet::FeatureSet(ConstFeaturePtr m, EntityType shape, const std::vector<FeatureID>& ids)
: model_(m),
  shape_(shape),
  data_(ids.begin(), ids.end())
{}

FeatureSet::~FeatureSet()
{}



FeatureSet::operator TopAbs_ShapeEnum () const
{
  if (shape_==Edge) return TopAbs_EDGE;
  else if (shape_==Face) return TopAbs_FACE;
  else if (shape_==Vertex) return TopAbs_VERTEX;
  else if (shape_==Solid) return TopAbs_SOLID;
  else throw insight::Exception("Unknown EntityType:"+lexical_cast<std::string>(shape_));
}

const FeatureSetData& FeatureSet::data() const
{
  return data_;
}

void FeatureSet::setData(const FeatureSetData& d)
{
  data_=d;
}

void FeatureSet::add(const FeatureID& e)
{
  data_.insert(e);
}



FeatureSet::operator const FeatureSetData& () const
{
  return data();
}


void FeatureSet::safe_union(const FeatureSet& o)
{
    data(); // trigger build
    if (o.shape()!=shape())
        throw insight::Exception("incompatible shape type between feature sets!");
    else if (!(o.model()==model()))
        throw insight::Exception("feature sets belong to different models!");
    else
    {
        data_.insert(o.data().begin(), o.data().end());
    }
}

void FeatureSet::safe_union(ConstFeatureSetPtr o)
{
  safe_union(*o);
}



size_t FeatureSet::size() const
{
  return data().size();
}



FeatureSetPtr FeatureSet::clone() const
{
  return std::make_shared<FeatureSet>(*this);
}

void FeatureSet::write() const
{
  std::cout<<'[';
    for (FeatureID i: data())
  {
    std::cout<<" "<<i;
  }
  std::cout<<" ]"<<std::endl;
}


size_t FeatureSet::calcFeatureSetHash() const
{
    ParameterListHash h;

    h += *model();
    h += int(shape());

    for (auto i: data())
    {
        h += i;
    }

    return h.getHash();
}


std::ostream& operator<<(std::ostream& os, const FeatureSetData& fsd)
{
  os<<fsd.size()<<" {";
  for (int fi: fsd)
  {
    os<<" "<<fi;
  }
  os<<" }";
  return os;
}

std::ostream& operator<<(std::ostream& os, const FeatureSet& fs)
{
  os << fs.data();
  return os;
}


size_t DeferredFeatureSet::calcHash() const
{
    return calcFeatureSetHash();
}

size_t DeferredFeatureSet::calcFeatureSetHash() const
{
    ParameterListHash h;

    h += *model();
    if (baseSet_)
        h += *baseSet_;
    h += int(shape());
    h += filterexpr_;

    for (const FeatureSetParserArg& arg: refs_)
    {
        if (auto *fp = boost::get<FeatureSetPtr>(&arg))
        {
            h += **fp;
        }
        else if (auto *v = boost::get<VectorPtr>(&arg))
        {
            h += (*v)->value();
        }
        else if (auto *s = boost::get<ScalarPtr>(&arg))
        {
            h += (*s)->value();
        }
        else
            throw insight::UnhandledSelection();
    }

    return h.getHash();
}


void DeferredFeatureSet::build()
{
    switch (shape())
    {
    case Vertex:
        if (!filterexpr_.empty())
        {
            if (baseSet_)
                setData(model()->query_vertices_subset(baseSet_->data(), filterexpr_, refs_));
            else
                setData(model()->query_vertices(filterexpr_, refs_));
        }
        else
            setData(model()->allVerticesSet());
        break;
    case Edge:
        if (!filterexpr_.empty())
        {
            if (baseSet_)
                setData(model()->query_edges_subset(baseSet_->data(), filterexpr_, refs_));
            else
                setData(model()->query_edges(filterexpr_, refs_));
        }
        else
            setData(model()->allEdgesSet());
        break;
    case Face:
        if (!filterexpr_.empty())
        {
            if (baseSet_)
                setData(model()->query_faces_subset(baseSet_->data(), filterexpr_, refs_));
            else
                setData(model()->query_faces(filterexpr_, refs_));
        }
        else
            setData(model()->allFacesSet());
        break;
    case Solid:
        if (!filterexpr_.empty())
        {
            if (baseSet_)
                setData(model()->query_solids_subset(baseSet_->data(), filterexpr_, refs_));
            else
                setData(model()->query_solids(filterexpr_, refs_));
        }
        else
            setData(model()->allSolidsSet());
        break;
    default:
        throw insight::Exception("Unknown feature type");
    }
}


DeferredFeatureSet::DeferredFeatureSet
    (
        ConstFeaturePtr m,
        EntityType shape,
        const string& filterexpr,
        const FeatureSetParserArgList& refs
        )
: FeatureSet(m, shape),
    filterexpr_(filterexpr),
    refs_(refs)
{}


DeferredFeatureSet::DeferredFeatureSet
    (
        ConstFeatureSetPtr q,
        const string& filterexpr,
        const FeatureSetParserArgList& refs
        )
: FeatureSet(q->model(), q->shape()),
    baseSet_(q),
    filterexpr_(filterexpr),
    refs_(refs)
{}


const FeatureSetData &DeferredFeatureSet::data() const
{
    checkForBuildDuringAccess();
    return FeatureSet::data();
}

FeatureSetPtr DeferredFeatureSet::clone() const
{
    if (baseSet_)
        return DeferredFeatureSet::create(baseSet_, filterexpr_, refs_);
    else
        return DeferredFeatureSet::create(model(), shape(), filterexpr_, refs_);
}




FeatureSetPtr makeVertexFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Vertex>(feat, expression, refs);
}

FeatureSetPtr makeEdgeFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Edge>(feat, expression, refs);
}


FeatureSetPtr makeFaceFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Face>(feat, expression, refs);
}


FeatureSetPtr makeSolidFeatureSet(
    ConstFeaturePtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Solid>(feat, expression, refs);
}




FeatureSetPtr makeVertexFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Vertex>(feat, expression, refs);
}

FeatureSetPtr makeEdgeFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Edge>(feat, expression, refs);
}


FeatureSetPtr makeFaceFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Face>(feat, expression, refs);
}


FeatureSetPtr makeSolidFeatureSet(
    ConstFeatureSetPtr feat,
    const std::string& expression,
    const FeatureSetParserArgList& refs
    )
{
    return makeFeatureSet<Solid>(feat, expression, refs);
}


}
}
