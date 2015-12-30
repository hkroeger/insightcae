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

Datum::Datum(istream& file)
{
  file>>providesPointReference_;
  file>>providesAxisReference_;
  file>>providesPlanarReference_;
}

Datum::~Datum()
{
}

gp_Pnt Datum::point() const
{
  throw insight::Exception("Not implemented: provide point reference");
  return gp_Pnt();
}

  
Datum::operator const gp_Pnt () const
{
  return point();
}

gp_Ax1 Datum::axis() const
{
  throw insight::Exception("Not implemented: provide axis reference");
  return gp_Ax1();
}


Datum::operator const gp_Ax1 () const
{
  return axis();
}

gp_Ax3 Datum::plane() const
{
  throw insight::Exception("Not implemented: provide planar reference");
  return gp_Ax3();
}


Datum::operator const gp_Ax3 () const
{
  return plane();
}

AIS_InteractiveObject* Datum::createAISRepr() const
{
  throw insight::Exception("Not implemented: provide AIS_InteractiveObject presentation");
  return NULL;
}

void Datum::write(ostream& file) const
{
  file<<providesPointReference_<<endl;
  file<<providesAxisReference_<<endl;
  file<<providesPlanarReference_<<endl;
}


void DatumPlane::build()
{
  arma::mat n=n_->value()/arma::norm(n_->value(),2);
  if (!up_)
  {

    arma::mat vx=cross(vec3(0,1,0), n); 
    double m=norm(vx, 2);
    if (m<1e-6)
    {
      vx=cross(vec3(1,0,0), n);
      m=norm(vx, 2);
    }
    vx/=m;
    
    cs_ = gp_Ax3( to_Pnt(p0_->value()), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
  }
  else
  {
    arma::mat vx=cross(up_->value(), n); 
    double m=norm(vx, 2);
    if (m<1e-6)
    {
      throw insight::Exception("normal and upward direction are aligned!");
    }
    vx/=m;

//     cout<<"p0="<<p0<<endl;
//     cout<<"n="<<n<<endl;
//     cout<<"vx="<<vx<<endl;
    
    cs_ = gp_Ax3( to_Pnt(p0_->value()), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
  }
}

  
DatumPlane::DatumPlane(VectorPtr p0, VectorPtr ni)
: Datum(true, false, true),
  p0_(p0),
  n_(ni)
{}

DatumPlane::DatumPlane(VectorPtr p0, VectorPtr ni, VectorPtr up)
: Datum(true, false, true),
  p0_(p0), n_(ni), up_(up)
{}

// DatumPlane::DatumPlane
// (
//   FeaturePtr m, 
//   FeatureID f
// )
// : Datum(true, false, true)
// {
//   arma::mat p0=m.faceCoG(f);
//   arma::mat n=m.faceNormal(f);
//   
//   n/=norm(n,2);
//   
//   arma::mat vx=cross(vec3(0,1,0), n); 
//   double mo=norm(vx, 2);
//   if (mo<1e-6)
//   {
//     vx=cross(vec3(1,0,0), n);
//     mo=norm(vx, 2);
//   }
//   vx/=mo;
//   
//   cs_ = gp_Ax3( to_Pnt(p0), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
// }

gp_Pnt DatumPlane::point() const
{
  checkForBuildDuringAccess();
  return cs_.Location();
}

gp_Ax3 DatumPlane::plane() const
{
  checkForBuildDuringAccess();
  return cs_;
}

// DatumPlane::operator const Handle_AIS_InteractiveObject () const
AIS_InteractiveObject* DatumPlane::createAISRepr() const
{
  checkForBuildDuringAccess();
  return new AIS_Plane(Handle_Geom_Plane(new Geom_Plane(cs_)));
}



void DatumPlane::write(ostream& file) const
{
  checkForBuildDuringAccess();
  insight::cad::Datum::write(file);
}


}
}