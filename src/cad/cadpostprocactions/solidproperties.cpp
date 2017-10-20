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

#include "cadfeature.h"
#include "solidproperties.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

using namespace boost;


insight::cad::SolidProperties::SolidProperties(insight::cad::FeaturePtr model)
: model_(model)
{}

void insight::cad::SolidProperties::build()
{
  cout<<"######### SolidProperties Report ###########################################"<<endl;
  mass_=model_->mass();
  cout<<"mass="<<mass_<<endl;

  area_=model_->modelSurfaceArea();
  cout<<"area="<<area_<<endl;
  
  cog_=model_->modelCoG();
  cout<<"CoG = ["<<endl
    <<cog_
    <<" ]"<<endl;
  
  inertia_=model_->modelInertia();
  cout<<"inertia tensor = [ "<<endl
       <<inertia_
       <<" ]"<<endl;
  
  arma::mat bb=model_->modelBndBox(/*0.01*/);
  bb_pmin_=bb.col(0);
  bb_pmax_=bb.col(1);
}

void insight::cad::SolidProperties::write(ostream&) const
{

}


Handle_AIS_InteractiveObject insight::cad::SolidProperties::createAISRepr(const Handle_AIS_InteractiveContext& context) const
{
  checkForBuildDuringAccess();
  
  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(cog_),gp_Dir(1,0,0)), 1));
  Handle_AIS_Shape aisG = new AIS_Shape(cG);
  context->Load(aisG);

//   Handle_AIS_InteractiveObject aisGLabel(createArrow(cG, str(format("CoG: m = %g, A = %g") % mass_ % area_)));
  Handle_AIS_InteractiveObject aisGLabel(new InteractiveText 
    (
      str(format("CoG: m = %g, A = %g") % mass_ % area_), cog_ //,
//       double angle = 0, double slant = 0,
//       int color_id = 1, int font_id = 1,
//       double scale = 0.2
    ));
  context->Load(aisGLabel);
    
  double 
   Lx=bb_pmax_(0)-bb_pmin_(0),
   Ly=bb_pmax_(1)-bb_pmin_(1),
   Lz=bb_pmax_(2)-bb_pmin_(2);

  gp_Pnt p11(bb_pmin_(0), bb_pmax_(1), bb_pmax_(2));
  gp_Pnt p12(bb_pmax_(0), bb_pmax_(1), bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimX(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p11),
    BRepBuilderAPI_MakeVertex(p12),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmax_), gp_Vec(0,0,1)))),
    Lx, 
    ""
//     str(format("Lx = %g (%g to %g)")%Lx%bb_pmin_(0)%bb_pmax_(0)).c_str()
  ));
  context->Load(aisDimX);
  Handle_AIS_InteractiveObject aisDimXLabel(new InteractiveText 
    (
      str(format("Lx = %g (%g to %g)")%Lx%bb_pmin_(0)%bb_pmax_(0)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p11)
    ));
  context->Load(aisDimXLabel);

  gp_Pnt p21(bb_pmax_(0), bb_pmin_(1), bb_pmax_(2));
  gp_Pnt p22(bb_pmax_(0), bb_pmax_(1), bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimY(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p21),
    BRepBuilderAPI_MakeVertex(p22),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmax_), gp_Vec(0,0,-1)))),
    Ly, 
    "" //str(format("Ly = %g (%g to %g)")%Ly%bb_pmin_(1)%bb_pmax_(1)).c_str()
  ));
  context->Load(aisDimY);
  Handle_AIS_InteractiveObject aisDimYLabel(new InteractiveText 
    (
      str(format("Ly = %g (%g to %g)")%Ly%bb_pmin_(1)%bb_pmax_(1)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p21)
    ));
  context->Load(aisDimYLabel);

  gp_Pnt p31(bb_pmax_(0), bb_pmax_(1), bb_pmin_(2));
  gp_Pnt p32(bb_pmax_(0), bb_pmax_(1), bb_pmax_(2));
  Handle_AIS_InteractiveObject aisDimZ(createLengthDimension(
    BRepBuilderAPI_MakeVertex(p31),
    BRepBuilderAPI_MakeVertex(p32),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmax_), gp_Vec(-1,1,0)))),
    Lz, 
    "" //str(format("Lz = %g (%g to %g)")%Lz%bb_pmin_(2)%bb_pmax_(2)).c_str()
  ));
  context->Load(aisDimZ);
  Handle_AIS_InteractiveObject aisDimZLabel(new InteractiveText 
    (
      str(format("Lz = %g (%g to %g)")%Lz%bb_pmin_(2)%bb_pmax_(2)).c_str(),
      //vec3(0.5*(p11.XYZ()+p12.XYZ()))
     vec3(p31)
    ));
  context->Load(aisDimZLabel);

  Handle_AIS_MultipleConnectedInteractive ais(new AIS_MultipleConnectedInteractive());
  context->Load(ais);
  
  ais->Connect(aisG);
  ais->Connect(aisGLabel);
  ais->Connect(aisDimX);
  ais->Connect(aisDimXLabel);
  ais->Connect(aisDimY);
  ais->Connect(aisDimYLabel);
  ais->Connect(aisDimZ);
  ais->Connect(aisDimZLabel);

  return ais;
}
