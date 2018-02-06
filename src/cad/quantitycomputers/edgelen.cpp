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

#include "edgelen.h"
#include "cadfeature.h"

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
edgeLen::edgeLen()
{
}

edgeLen::~edgeLen()
{
}

double edgeLen::evaluate(FeatureID ei)
{
  GProp_GProps gpr;
  TopoDS_Edge e=model_->edge(ei);
  double l = 0.;
  if (!e.IsNull() && !BRep_Tool::Degenerated(e)) {
    BRepGProp::LinearProperties(e, gpr);
    l = gpr.Mass();
  }
  return l;
}

QuantityComputer< double >::Ptr edgeLen::clone() const
{
  return QuantityComputer<double>::Ptr(new edgeLen());
}




circRadius::circRadius()
{
}

circRadius::~circRadius()
{
}

double circRadius::evaluate(FeatureID ei)
{
  TopoDS_Edge e=model_->edge(ei);
  if (!e.IsNull() && !BRep_Tool::Degenerated(e))
    {
      double p0, p1;
      GeomAdaptor_Curve adapt(BRep_Tool::Curve(e, p0, p1));
      if (adapt.GetType()==GeomAbs_Circle)
        {
          gp_Circ ic=adapt.Circle();
          return ic.Radius();
        }
    }
  return 0;
}

QuantityComputer< double >::Ptr circRadius::clone() const
{
  return QuantityComputer<double>::Ptr(new circRadius());
}


}
}
