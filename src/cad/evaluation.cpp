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

namespace insight {
namespace cad {

SolidProperties::SolidProperties(const SolidModel& model)
{
  cog_=model.modelCoG();
  cout<<"CoG="<<cog_<<endl;
}

void SolidProperties::write(std::ostream&) const
{
}

AIS_InteractiveObject* SolidProperties::createAISRepr() const
{
  return new AIS_Point(new Geom_CartesianPoint(to_Pnt(cog_)));
}


Hydrostatics::Hydrostatics
(
  const SolidModel& hull_volume, 
  const arma::mat& psurf, const arma::mat& nsurf,
  const arma::mat& elong, const arma::mat& evert
)
{
  Cutaway submerged_volume(hull_volume, psurf, nsurf);
  V_=submerged_volume.modelVolume();
  cout<<"displacement V="<<V_<<endl;
  
  SolidModelPtr csf = submerged_volume.providedSubshapes().at("CutSurface");
  if (!csf)
    throw insight::Exception("No cut surface present!");
  cout<<"ok1!"<<endl;
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

  G_ = hull_volume.modelCoG();
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
  Handle_AIS_Point aisG (new AIS_Point(new Geom_CartesianPoint(to_Pnt(G_))));
  Handle_AIS_Point aisB (new AIS_Point(new Geom_CartesianPoint(to_Pnt(B_))));
  Handle_AIS_Point aisM (new AIS_Point(new Geom_CartesianPoint(to_Pnt(M_))));
  Handle_AIS_InteractiveObject aisDim (new AIS_LengthDimension(
    BRepBuilderAPI_MakeVertex(to_Pnt(G_)), BRepBuilderAPI_MakeVertex(to_Pnt(M_)),
    Handle_Geom_Plane(new Geom_Plane(gp_Pln(to_Pnt(G_), to_Vec(vec3(0,1,0))))),
    norm(G_-M_,2), "GM"
  ));

  std::auto_ptr<AIS_MultipleConnectedInteractive> ais(new AIS_MultipleConnectedInteractive());
  ais->Connect(aisG);
  ais->Connect(aisB);
  ais->Connect(aisM);
  ais->Connect(aisDim);
  
  return ais.release();
}


}
}