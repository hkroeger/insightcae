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
: ASTBase(o),
  model_(o.model_),
  base_set_(o.base_set_),
  shape_(o.shape_),
  filterexpr_(o.filterexpr_),
  refs_(o.refs_)
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

FeatureSet::FeatureSet
(
  ConstFeaturePtr m, 
  EntityType shape, 
  const string& filterexpr, 
  const FeatureSetParserArgList& refs
)
: model_(m),
  shape_(shape),
  filterexpr_(filterexpr),
  refs_(refs)
{}


FeatureSet::FeatureSet
(
  ConstFeatureSetPtr q, 
  const string& filterexpr, 
  const FeatureSetParserArgList& refs
)
: model_(q->model()),
  shape_(q->shape()),
  base_set_(q),
  filterexpr_(filterexpr),
  refs_(refs)
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
  checkForBuildDuringAccess();
  return data_;
}

void FeatureSet::setData(const FeatureSetData& d)
{
  data_=d;
  setValid();
}

void FeatureSet::add(const FeatureID& e)
{
  data_.insert(e);
  setValid();
}



FeatureSet::operator const FeatureSetData& () const
{
  return data();
}


void FeatureSet::safe_union(const FeatureSet& o)
{
  if (o.shape_!=shape_)
    throw insight::Exception("incompatible shape type between feature sets!");
  else if (!(o.model_==model_))
    throw insight::Exception("feature sets belong to different models!");
  else
  {
    checkForBuildDuringAccess();
    data_.insert(o.data().begin(), o.data().end());
  }
}

void FeatureSet::safe_union(ConstFeatureSetPtr o)
{
  safe_union(*o);
}

void FeatureSet::build()
{
  switch (shape_)
  {
    case Vertex:
      if (!filterexpr_.empty())
      {
	if (base_set_)
	  data_=model_->query_vertices_subset(base_set_->data(), filterexpr_, refs_);
	else
	  data_=model_->query_vertices(filterexpr_, refs_);
      }
      else
	data_=model_->allVerticesSet();
      break;
    case Edge:
      if (!filterexpr_.empty())
      {
	if (base_set_)
	  data_=model_->query_edges_subset(base_set_->data(), filterexpr_, refs_);
	else
	  data_=model_->query_edges(filterexpr_, refs_);
      }
      else
	data_=model_->allEdgesSet();
      break;
    case Face:
      if (!filterexpr_.empty())
      {
	if (base_set_)
	  data_=model_->query_faces_subset(base_set_->data(), filterexpr_, refs_);
	else
	  data_=model_->query_faces(filterexpr_, refs_);
      }
      else
	data_=model_->allFacesSet();
      break;
    case Solid:
      if (!filterexpr_.empty())
      {
	if (base_set_)
	  data_=model_->query_solids_subset(base_set_->data(), filterexpr_, refs_);
	else
	  data_=model_->query_solids(filterexpr_, refs_);
      }
      else
	data_=model_->allSolidsSet();
      break;
    default:
      throw insight::Exception("Unknown feature type");
  }
}

size_t FeatureSet::size() const
{
  return data().size();
}


// FeatureSet FeatureSet::query(const FilterPtr& f) const
// {
//   switch (shape_)
//   {
//     case Vertex:
//       return model_.query_vertices_subset(*this, f);
//       break;
//     case Edge:
//       return model_.query_edges_subset(*this, f);
//       break;
//     case Face:
//       return model_.query_faces_subset(*this, f);
//       break;
//     case Solid:
//       return model_.query_solids_subset(*this, f);
//       break;
//     default:
//       throw insight::Exception("Unknown feature type");
//   }
// }
// 
// FeatureSet FeatureSet::query(const std::string& queryexpr) const
// {
//   std::istringstream is(queryexpr);
//   switch (shape_)
//   {
//     case Vertex:
//       return model_.query_vertices_subset(*this, parseVertexFilterExpr(is));
//       break;
//     case Edge:
//       return model_.query_edges_subset(*this, parseEdgeFilterExpr(is));
//       break;
//     case Face:
//       return model_.query_faces_subset(*this, parseFaceFilterExpr(is));
//       break;
//     case Solid:
//       return model_.query_solids_subset(*this, parseSolidFilterExpr(is));
//       break;
//     default:
//       throw insight::Exception("Unknown feature type");
//   }
// }


FeatureSetPtr FeatureSet::clone() const
{
//checkForBuildDuringAccess();
  FeatureSetPtr nfs(new FeatureSet(*this));
  return nfs;
}

void FeatureSet::write() const
{
  std::cout<<'[';
  BOOST_FOREACH(FeatureID i, data_)
  {
    std::cout<<" "<<i;
  }
  std::cout<<" ]"<<std::endl;
}

size_t FeatureSet::hash() const
{
    size_t h=0;
    boost::hash_combine(h, *model_);
    if (base_set_) boost::hash_combine(h, *base_set_);
    boost::hash_combine(h, int(shape_));
    boost::hash_combine(h, filterexpr_);
    BOOST_FOREACH(const FeatureSetParserArg& arg, refs_)
    {
        if (const FeatureSetPtr *fp = boost::get<FeatureSetPtr>(&arg))
        {
            boost::hash_combine(h, **fp);
        }
    }
    return h;
}

std::ostream& operator<<(std::ostream& os, const FeatureSetData& fsd)
{
  os<<fsd.size()<<" {";
  BOOST_FOREACH(int fi, fsd)
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

}
}
