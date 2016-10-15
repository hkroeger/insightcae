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

#include "occinclude.h"
#include "projectedpoint.h"
#include "datum.h"
#include "cadfeature.h"

#include "GeomAPI_IntCS.hxx"
#include "Geom_Line.hxx"

#include "BRepIntCurveSurface_Inter.hxx"

namespace insight {
namespace cad {
  
ProjectedPoint::ProjectedPoint(VectorPtr p0, DatumPtr plane)
: p0_(p0), plane_(plane)
{}

ProjectedPoint::ProjectedPoint(VectorPtr p0, DatumPtr plane, VectorPtr along)
: p0_(p0), plane_(plane), along_(along)
{}

arma::mat ProjectedPoint::value() const
{
  if (!plane_->providesPlanarReference())
    throw insight::Exception("Given datum does not provide a plane!");
      
  if (!along_)
  {  
    Handle_Geom_Plane pl(new Geom_Plane(plane_->plane()));
    GeomAPI_ProjectPointOnSurf proj(to_Pnt(p0_->value()), pl);
    gp_Pnt p=proj.NearestPoint();
    return vec3(p.X(), p.Y(), p.Z());
  }
  else
  {
    arma::mat dir=along_->value();
    
    gp_Dir d(dir(0), dir(1), dir(2));
    Handle_Geom_Curve l(new Geom_Line( to_Pnt(p0_->value()), d ));
    Handle_Geom_Surface pl(new Geom_Plane(plane_->plane()));

    GeomAPI_IntCS isec(l, pl);
    
    double nearest=DBL_MAX;
    arma::mat res;
    for (int i=1; i<=isec.NbPoints(); i++)
    {
      gp_Pnt p=isec.Point(i);
      arma::mat loc=vec3(p.X(), p.Y(), p.Z());
      double dist=norm( loc - p0_->value(), 2);
      if (dist<nearest) 
      {
	nearest=dist;
	res=loc;
      }
    }
    
    if (isec.NbPoints()<1)
      throw insight::Exception("projection not successful!");
    
    return res;
  }
}


ProjectedPointOnFeature::ProjectedPointOnFeature(VectorPtr p0, VectorPtr along, FeaturePtr targ)
    : p0_(p0), along_(along), targ_(targ)
{}

arma::mat ProjectedPointOnFeature::value() const
{
    gp_Pnt p0=to_Pnt(p0_->value());
    gp_Pnt pproj;
    
    double nearestdist=DBL_MAX;
    bool found=false;
    BRepIntCurveSurface_Inter proj;
    for 
    (
        proj.Init
        (
            targ_->shape(),
            gp_Lin(p0, gp_Dir(to_Vec(along_->value()))),
            Precision::Confusion()
        );
        proj.More(); 
        proj.Next()
    )
    {
        gp_Pnt cp = proj.Pnt();
        double d=cp.Distance(p0);
        std::cout<<"dist="<<d<<", nearest="<<nearestdist<<std::endl;
        if (d<nearestdist)
        {
            nearestdist=d;
            pproj=cp;
            found=true;
        }
    }

    if (!found)
        throw insight::Exception("ProjectedPointOnFeature: did not find a projection!");

    arma::mat resp;
    resp << pproj.X()<<arma::endr << pproj.Y()<<arma::endr << pproj.Z();
    return resp;
}

}
}
