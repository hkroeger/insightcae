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

#include "evaluation.h"
#include "solidmodel.h"

#include "AIS_Point.hxx"

using namespace boost;

namespace insight {
namespace cad {

SolidProperties::SolidProperties(const SolidModel& model)
{
  cog_=model.modelCoG();
  cout<<"CoG="<<cog_<<endl;
  
  mass_=model.mass();
  cout<<"mass="<<mass_<<endl;

  area_=model.modelSurfaceArea();
  cout<<"area="<<area_<<endl;
  
  arma::mat bb=model.modelBndBox(0.01);
  bb_pmin_=bb.col(0);
  bb_pmax_=bb.col(1);
}

void SolidProperties::write(std::ostream&) const
{
}

AIS_InteractiveObject* SolidProperties::createAISRepr() const
{
  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(cog_),gp_Dir(1,0,0)), 1));
  Handle_AIS_Shape aisG = new AIS_Shape(cG);

  Handle_AIS_InteractiveObject aisGLabel (new AIS_RadiusDimension(
    cG, 1e-6, str(format("CoG: m = %g, A = %g") % mass_ % area_).c_str()
  ));

  double 
   Lx=bb_pmax_(0)-bb_pmin_(0),
   Ly=bb_pmax_(1)-bb_pmin_(1),
   Lz=bb_pmax_(2)-bb_pmin_(2);
   
  Handle_AIS_InteractiveObject aisDimX(new AIS_LengthDimension(
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmin_(0), bb_pmin_(1), bb_pmin_(2))),
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmax_(0), bb_pmin_(1), bb_pmin_(2))),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmin_), gp_Vec(0,0,1)))),
    Lx, 
    str(format("Lx = %g (%g to %g)")%Lx%bb_pmin_(0)%bb_pmax_(0)).c_str()
  ));

  Handle_AIS_InteractiveObject aisDimY(new AIS_LengthDimension(
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmin_(0), bb_pmin_(1), bb_pmin_(2))),
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmin_(0), bb_pmax_(1), bb_pmin_(2))),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmin_), gp_Vec(0,0,-1)))),
    Ly, 
    str(format("Ly = %g (%g to %g)")%Ly%bb_pmin_(1)%bb_pmax_(1)).c_str()
  ));

  Handle_AIS_InteractiveObject aisDimZ(new AIS_LengthDimension(
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmin_(0), bb_pmin_(1), bb_pmin_(2))),
    BRepBuilderAPI_MakeVertex(gp_Pnt(bb_pmin_(0), bb_pmin_(1), bb_pmax_(2))),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(bb_pmin_), gp_Vec(-1,1,0)))),
    Lz, 
    str(format("Lz = %g (%g to %g)")%Lz%bb_pmin_(2)%bb_pmax_(2)).c_str()
  ));

  std::auto_ptr<AIS_MultipleConnectedInteractive> ais(new AIS_MultipleConnectedInteractive());
  ais->Connect(aisG);
  ais->Connect(aisGLabel);
  ais->Connect(aisDimX);
  ais->Connect(aisDimY);
  ais->Connect(aisDimZ);

  return ais.release();
}


Hydrostatics::Hydrostatics
(
  const SolidModel& hull_volume, 
  const SolidModel& shipmodel,
  const arma::mat& psurf, const arma::mat& nsurf,
  const arma::mat& elong, const arma::mat& evert
)
{
  elat_=arma::cross(nsurf, elong);
  
  Cutaway submerged_volume(hull_volume, psurf, nsurf);
  V_=submerged_volume.modelVolume();
  cout<<"displacement V="<<V_<<endl;
  
  m_=shipmodel.mass();
  cout<<"ship mass m="<<m_<<endl;

  SolidModelPtr csf = submerged_volume.providedSubshapes().at("CutSurface");
  if (!csf)
    throw insight::Exception("No cut surface present!");

  // write surface for debug
  TopoDS_Shape issh=static_cast<const TopoDS_Shape&>(*csf);
  BRepTools::Write(issh, "test.brep");
  
  GProp_GProps props;
  BRepGProp::SurfaceProperties(issh, props);
  GProp_PrincipalProps pcp = props.PrincipalProperties();
  arma::mat I=arma::zeros(3);
  pcp.Moments(I(0), I(1), I(2));
  cout<<"I="<<I<<endl;
  double BM = I.min()/V_;
  cout<<"BM="<<BM<<endl;

  G_ = shipmodel.modelCoG();
  B_ = submerged_volume.modelCoG();
  M_ = B_ + BM*evert;
  double GM = norm(M_ - G_, 2);
  cout<<"G="<<G_<<endl;
  cout<<"B="<<B_<<endl;
  cout<<"M="<<M_<<endl;
  cout<<"GM="<<GM<<endl;
}

void Hydrostatics::write(ostream&) const
{

}


AIS_InteractiveObject* Hydrostatics::createAISRepr() const
{
  TopoDS_Edge cG = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(G_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisG = new AIS_Shape(cG);
  TopoDS_Edge cB = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(B_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisB = new AIS_Shape(cB);
  TopoDS_Edge cM = BRepBuilderAPI_MakeEdge(gp_Circ(gp_Ax2(to_Pnt(M_),gp_Dir(to_Vec(elat_))), 1));
  Handle_AIS_Shape aisM = new AIS_Shape(cM);
  
  double d_gm=norm(G_-M_,2);
  Handle_AIS_InteractiveObject aisDim (new AIS_LengthDimension(
    BRepBuilderAPI_MakeVertex(to_Pnt(G_)), BRepBuilderAPI_MakeVertex(to_Pnt(M_)),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(G_), to_Vec(vec3(0,1,0))))),
    d_gm, 
    str(format("GM = %g")%d_gm).c_str()
  ));
  

  Handle_AIS_InteractiveObject aisBLabel (new AIS_RadiusDimension(
    cB, 1e-6, str(format("B: V_disp = %g") % V_).c_str()
  ));
  Handle_AIS_InteractiveObject aisGLabel (new AIS_RadiusDimension(
    cG, 1e-6, str(format("G: m = %g") % m_).c_str()
  ));

  std::auto_ptr<AIS_MultipleConnectedInteractive> ais(new AIS_MultipleConnectedInteractive());
  ais->Connect(aisG);
  ais->Connect(aisB);
  ais->Connect(aisM);
  ais->Connect(aisDim);
  ais->Connect(aisGLabel);
  ais->Connect(aisBLabel);
  
  return ais.release();
}


}
}