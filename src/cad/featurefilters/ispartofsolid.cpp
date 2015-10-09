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

#include "ispartofsolid.h"
#include "solidmodel.h"

#include "occtools.h"
#include "geotest.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


template<> isPartOfSolid<Edge>::isPartOfSolid(const SolidModel& m)
: s_(TopoDS::Solid(m))
{
}

template<>
bool isPartOfSolid<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
  match |= isPartOf(s_, e1);
  
  return match;
}

template<> isPartOfSolid<Face>::isPartOfSolid(const SolidModel& m)
: s_(TopoDS::Solid(m))
{
}

template<>
bool isPartOfSolid<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
//   BOOST_FOREACH(int f, f_)
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    match |= isPartOf(s_, e1);
  }
  
  return match;
}


}
}