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

#include "boundaryedge.h"
#include "solidmodel.h"
#include "occinclude.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{

boundaryEdge::boundaryEdge()
{
}

void boundaryEdge::initialize(const SolidModel& m)
{
    insight::cad::Filter::initialize(m);
    safb_.reset(new ShapeAnalysis_FreeBounds(m));
}


bool boundaryEdge::checkMatch(FeatureID feature) const
{
  for (TopExp_Explorer ex(safb_->GetOpenWires(), TopAbs_EDGE); ex.More(); ex.Next())
  {
    if (TopoDS::Edge(ex.Current()).IsEqual(model_->edge(feature)))
      return true;
  }
  for (TopExp_Explorer ex(safb_->GetClosedWires(), TopAbs_EDGE); ex.More(); ex.Next())
  {
    if (TopoDS::Edge(ex.Current()).IsEqual(model_->edge(feature)))
      return true;
  }
  return false;
}

FilterPtr boundaryEdge::clone() const
{
  return FilterPtr(new boundaryEdge());  
}

}
}
