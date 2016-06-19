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
#include "AIS_Axis.hxx"
#include "Geom_Axis1Placement.hxx"
#include "GeomAPI_IntSS.hxx"
#include "GeomAPI_IntCS.hxx"
#include "Geom_Line.hxx"

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



DatumPoint::DatumPoint()
: Datum(true, false, false)
{}

gp_Pnt DatumPoint::point() const
{
  checkForBuildDuringAccess();
  return p_;
}

AIS_InteractiveObject* DatumPoint::createAISRepr() const
{
  return new AIS_Shape( BRepBuilderAPI_MakeVertex(point()) );
}

ProvidedDatum::ProvidedDatum(FeaturePtr feat, std::string name)
: Datum(false, false, false),
  feat_(feat), 
  name_(name)
{}

void ProvidedDatum::build()
{
  auto iter=feat_->providedDatums().find(name_);
  if (iter==feat_->providedDatums().end())
    throw insight::Exception("Feature does not provide a datum of name \""+name_+"\"");
  dat_=iter->second;
  providesPointReference_=dat_->providesPointReference();
  providesAxisReference_=dat_->providesAxisReference();
  providesPlanarReference_=dat_->providesPlanarReference();
}

gp_Pnt ProvidedDatum::point() const
{
  checkForBuildDuringAccess();
  return dat_->point();
}

gp_Ax1 ProvidedDatum::axis() const
{
  checkForBuildDuringAccess();
  return dat_->axis();
}

gp_Ax3 ProvidedDatum::plane() const
{
  checkForBuildDuringAccess();
  return dat_->plane();
}

AIS_InteractiveObject* ProvidedDatum::createAISRepr() const
{
    return dat_->createAISRepr();
}


ExplicitDatumPoint::ExplicitDatumPoint(VectorPtr c)
: coord_(c)
{}


void ExplicitDatumPoint::build()
{
  p_=to_Pnt(coord_->value());
}


DatumAxis::DatumAxis()
: Datum(true, true, false)
{}


gp_Pnt DatumAxis::point() const
{
  checkForBuildDuringAccess();
  return ax_.Location();
}

gp_Ax1 DatumAxis::axis() const
{
  checkForBuildDuringAccess();
  return ax_;
}

AIS_InteractiveObject* DatumAxis::createAISRepr() const
{
  checkForBuildDuringAccess();
  AIS_Axis *ais=new AIS_Axis(Handle_Geom_Axis1Placement(new Geom_Axis1Placement(axis())));
//   ais->SetWidth(100);
  return ais;
}

ExplicitDatumAxis::ExplicitDatumAxis(VectorPtr p0, VectorPtr ex)
: p0_(p0), ex_(ex)
{}

void ExplicitDatumAxis::build()
{
  ax_ = gp_Ax1( to_Pnt(p0_->value()), gp_Dir(to_Vec(ex_->value())) );
}


DatumPlaneData::DatumPlaneData()
: Datum(true, false, true)
{}


gp_Pnt DatumPlaneData::point() const
{
  checkForBuildDuringAccess();
  return cs_.Location();
}

gp_Ax3 DatumPlaneData::plane() const
{
  checkForBuildDuringAccess();
  return cs_;
}

// DatumPlane::operator const Handle_AIS_InteractiveObject () const
AIS_InteractiveObject* DatumPlaneData::createAISRepr() const
{
  checkForBuildDuringAccess();
  AIS_Plane *ais=new AIS_Plane(Handle_Geom_Plane(new Geom_Plane(plane())));
  ais->SetSize(100);
  return ais;
}


void DatumPlane::build()
{
  arma::mat n=n_->value()/arma::norm(n_->value(),2);
  if (p2_)
  {

    arma::mat vx=p1_->value() - p0_->value(); 
    arma::mat vy=p2_->value() - p0_->value(); 
    vx/=norm(vx, 2);
    vy/=norm(vy, 2);
    arma::mat n=cross(vx, vy);
    n/=norm(n, 2);
    
    cs_ = gp_Ax3( to_Pnt(p0_->value()), gp_Dir(to_Vec(n)), gp_Dir(to_Vec(vx)) );
  }
  else if (!up_)
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
: p0_(p0),
  n_(ni)
{}

DatumPlane::DatumPlane(VectorPtr p0, VectorPtr ni, VectorPtr up)
: p0_(p0), n_(ni), up_(up)
{}

DatumPlane::DatumPlane(VectorPtr p0, VectorPtr p1, VectorPtr p2, bool dummy)
: p0_(p0), p1_(p1_), p2_(p2)
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



void DatumPlane::write(ostream& file) const
{
  checkForBuildDuringAccess();
  insight::cad::Datum::write(file);
}


XsecPlanePlane::XsecPlanePlane(ConstDatumPtr pl1, ConstDatumPtr pl2)
: pl1_(pl1), pl2_(pl2)
{}


void XsecPlanePlane::build()
{
  if (!pl1_->providesPlanarReference())
    throw insight::Exception("plane 1 does not provide plane reference!");
  if (!pl2_->providesPlanarReference())
    throw insight::Exception("plane 2 does not provide plane reference!");

  Handle_Geom_Plane plane_1 = new Geom_Plane(pl1_->plane());
  Handle_Geom_Plane plane_2 = new Geom_Plane(pl2_->plane());

  // Intersection
  GeomAPI_IntSS	intersection;
  intersection.Perform(plane_1, plane_2, 1.0e-7);

  // For debugging only
  if (!intersection.IsDone() || (intersection.NbLines()!=1) )
    throw insight::Exception("no intersection found!");
  
  // Get intersection curve
  gp_Lin xsec = Handle_Geom_Line::DownCast(intersection.Line(1))->Lin();
  
  ax_=gp_Ax1( xsec.Position()/*, xsec.Direction()*/ );
}



XsecAxisPlane::XsecAxisPlane(ConstDatumPtr ax, ConstDatumPtr pl)
: ax_(ax), pl_(pl)
{}

void XsecAxisPlane::build()
{
  if (!ax_->providesAxisReference())
    throw insight::Exception("axis reference does not provide axis!");
  if (!pl_->providesPlanarReference())
    throw insight::Exception("plane reference does not provide plane!");

  Handle_Geom_Line line_1 = new Geom_Line(ax_->axis());
  Handle_Geom_Plane plane_2 = new Geom_Plane(pl_->plane());

  // Intersection
  GeomAPI_IntCS	intersection;
  intersection.Perform(line_1, plane_2/*, 1.0e-7*/);

  // For debugging only
  if (!intersection.IsDone() || (intersection.NbPoints()!=1) )
    throw insight::Exception("no intersection found!");
  
  // Get intersection curve
  p_=intersection.Point(1);
//   std::cout<<"XSEC="<<p_.X()<<" "<<p_.Y()<<" "<<p_.Z()<<std::endl;
}

}
}