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

#include "faceadjacenttoedges.h"
#include "solidmodel.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{

faceAdjacentToEdges::faceAdjacentToEdges(FeatureSet edges)
: edges_(edges)
{
}

bool faceAdjacentToEdges::checkMatch(FeatureID feature) const
{
  TopoDS_Face f=model_->face(feature);
  for(TopExp_Explorer ex(f, TopAbs_EDGE); ex.More(); ex.Next())
  {
    TopoDS_Edge e=TopoDS::Edge(ex.Current());
    BOOST_FOREACH(FeatureID ei, edges_)
    {
      TopoDS_Edge e2=edges_.model().edge(ei);
      if (e.IsSame(e2))
	return true;
    }
  }
  return false;
}

FilterPtr faceAdjacentToEdges::clone() const
{
  return FilterPtr(new faceAdjacentToEdges(edges_));
}

}
}