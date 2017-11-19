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

#include "coincident.h"
#include "cadfeature.h"

#include "occtools.h"
#include "geotest.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


template<> coincident<Edge>::coincident(FeaturePtr m, scalarQuantityComputer::Ptr tol)
: f_(m->allEdges()),
  tol_(tol)
{
}

template<>
bool coincident<Edge>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_.data())
  {
    TopoDS_Edge e1=TopoDS::Edge(model_->edge(feature));
    TopoDS_Edge e2=TopoDS::Edge(f_.model()->edge(f));
    match |= isPartOf(e2, e1, tol_->evaluate(feature) );
  }
  
  return match;
}

template<> coincident<Face>::coincident(FeaturePtr m, scalarQuantityComputerPtr tol)
: f_(m->allFaces()),
  tol_(tol)
{
}

template<>
bool coincident<Face>::checkMatch(FeatureID feature) const
{
  bool match=false;
  
  BOOST_FOREACH(int f, f_.data())
  {
    TopoDS_Face e1=TopoDS::Face(model_->face(feature));
    TopoDS_Face e2=TopoDS::Face(f_.model()->face(f));
    match |= isPartOf(e2, e1, tol_->evaluate(feature) );
  }
  
  return match;
}

}
}
