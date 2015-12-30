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

#include "cylfaceorientation.h"
#include "cadfeature.h"


using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


cylFaceOrientation::cylFaceOrientation(bool io)
: io_(io)
{
}

bool cylFaceOrientation::checkMatch(FeatureID feature) const
{
  if (model_->faceType(feature)==GeomAbs_Cylinder)
  {
      GeomAdaptor_Surface adapt(BRep_Tool::Surface(model_->face(feature)));
      gp_Cylinder icyl=adapt.Cylinder();
      gp_Ax1 iax=icyl.Axis();
      BRepGProp_Face prop(model_->face(feature));
      double u1,u2,v1,v2;
      prop.Bounds(u1, u2, v1, v2);
      double u = (u1+u2)/2;
      double v = (v1+v2)/2;
      gp_Vec vec;
      gp_Pnt pnt;
      prop.Normal(u,v,pnt,vec);
      vec.Normalize();
      gp_XYZ dp=pnt.XYZ()-icyl.Location().XYZ();
      gp_XYZ ax=iax.Direction().XYZ();
      ax.Normalize();
      gp_XYZ dr=dp-ax.Multiplied(dp.Dot(ax));
      dr.Normalize();
      
      if (io_)
      {
	if (! (fabs(vec.XYZ().Dot(dr) + 1.) < 1e-6) ) return true;
      }
      else
      {
	if (! (fabs(vec.XYZ().Dot(dr) - 1.) < 1e-6) ) return true;
      }
  }
  else
    return false;
}

FilterPtr cylFaceOrientation::clone() const
{
  return FilterPtr(new cylFaceOrientation(io_));
}

}
}