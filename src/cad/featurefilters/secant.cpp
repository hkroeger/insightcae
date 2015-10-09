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

#include "solidmodel.h"
#include "secant.h"


using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
template<>
bool secant<Edge>::checkMatch(FeatureID feature) const
{
  TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));

  TopoDS_Vertex v0=TopExp::FirstVertex(e1);
  TopoDS_Vertex v1=TopExp::LastVertex(e1);
  arma::mat v = Vector( BRep_Tool::Pnt(v0).XYZ() - BRep_Tool::Pnt(v1).XYZ() );
  
  return (1.0 - fabs(arma::dot( v/arma::norm(v,2), dir_/arma::norm(dir_,2) ))) < 1e-10;
}

}
}