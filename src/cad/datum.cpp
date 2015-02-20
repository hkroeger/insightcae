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

#include "datum.h"

#include "AIS_Plane.hxx"

namespace insight {
namespace cad {

Datum::Datum(bool point, bool axis, bool planar)
: providesPointReference_(point),
  providesAxisReference_(axis),
  providesPlanarReference_(planar)
{
}

Datum::~Datum()
{
}
  
Datum::operator const gp_Pnt () const
{
  throw insight::Exception("Not implemented: provide point reference");
  return gp_Pnt();
}

Datum::operator const gp_Ax1 () const
{
  throw insight::Exception("Not implemented: provide axis reference");
  return gp_Ax1();
}

Datum::operator const gp_Ax3 () const
{
  throw insight::Exception("Not implemented: provide planar reference");
  return gp_Ax3();
}

Datum::operator const Handle_AIS_InteractiveObject () const
{
  throw insight::Exception("Not implemented: provide AIS_InteractiveObject presentation");
  return Handle_AIS_InteractiveObject();
}
  
DatumPlane::DatumPlane(const arma::mat& p0, const arma::mat& ni)
: Datum(true, false, true)
{
  arma::mat n=ni/norm(ni,2);

  arma::mat vx=cross(vec3(0,1,0), n); 
  double m=norm(vx, 2);
  if (m<1e-6)
  {
    vx=cross(vec3(1,0,0), n);
    m=norm(vx, 2);
  }
  vx/=m;
  
  cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
}

DatumPlane::DatumPlane(const arma::mat& p0, const arma::mat& ni, const arma::mat& up)
: Datum(true, false, true)
{
  arma::mat n=ni/norm(ni,2);

  arma::mat vx=cross(up, n); 
  double m=norm(vx, 2);
  if (m<1e-6)
  {
    throw insight::Exception("normal and upward direction are aligned!");
  }
  vx/=m;

  cout<<"p0="<<p0<<endl;
  cout<<"n="<<n<<endl;
  cout<<"vx="<<vx<<endl;
  
  cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
}

DatumPlane::DatumPlane
(
  const SolidModel& m, 
  FeatureID f
)
: Datum(true, false, true)
{
  arma::mat p0=m.faceCoG(f);
  arma::mat n=m.faceNormal(f);
  
  n/=norm(n,2);
  
  arma::mat vx=cross(vec3(0,1,0), n); 
  double mo=norm(vx, 2);
  if (mo<1e-6)
  {
    vx=cross(vec3(1,0,0), n);
    mo=norm(vx, 2);
  }
  vx/=mo;
  
  cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
}

DatumPlane::operator const gp_Pnt () const
{
  return cs_.Location();
}

DatumPlane::operator const gp_Ax3 () const
{
  return cs_;
}

DatumPlane::operator const Handle_AIS_InteractiveObject () const
{
  return Handle_AIS_InteractiveObject(new AIS_Plane(Handle_Geom_Plane(new Geom_Plane(cs_))));
}

}
}