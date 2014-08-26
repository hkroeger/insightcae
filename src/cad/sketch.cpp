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

#include "sketch.h"
#include "base/exception.h"

#include "TColStd_Array1OfInteger.hxx"

namespace insight {
namespace cad {

DXFReader::DXFReader(const boost::filesystem::path& filename, const std::string& layername)
: layername_(layername)
{
  std::auto_ptr<DL_Dxf> dxf(new DL_Dxf());
  if (!dxf->in(filename.c_str(), this)) 
  {
    std::cerr << filename << " could not be opened." << std::endl;
  }
}

DXFReader::~DXFReader()
{
}

void DXFReader::addArc(const DL_ArcData &a)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt cp(a.cx, a.cy, a.cz);
    gp_Circ c = gce_MakeCirc( gp_Ax2(cp, gp_Dir(0,0,1)), a.radius );
    gp_Pnt p0(cp); p0.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle1/180.), a.radius*::sin(M_PI*a.angle1/180.), 0) );
    gp_Pnt p1(cp); p1.Translate(gp_Vec(a.radius*::cos(M_PI*a.angle2/180.), a.radius*::sin(M_PI*a.angle2/180.), 0) );
//     Standard_Real Alpha1 = ElCLib::Parameter(c, p0);
//     Standard_Real Alpha2 = ElCLib::Parameter(c, p1);
//     Handle(Geom_Circle) C = new Geom_Circle(c);
//     Handle(Geom_TrimmedCurve) arc = new Geom_TrimmedCurve(C, Alpha1, Alpha2, false);
    cout<<a.cx<<" "<<a.cy<<" "<<a.cz<<endl;
    cout<<a.radius<<" "<<a.angle1<<" "<<a.angle2<<endl;
    cout<<p0.X()<<" "<<p0.Y()<<" "<<p0.Z()<<endl;
    cout<<p1.X()<<" "<<p1.Y()<<" "<<p1.Z()<<endl;
    cout<<"added arc"<<endl;
//     TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c, BRepBuilderAPI_MakeVertex(p0), BRepBuilderAPI_MakeVertex(p1)).Edge();
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c, p0, p1).Edge();
    ls_.Append(e);
  }
}

void DXFReader::addLine (const DL_LineData &l)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt p0(l.x1, l.y1, l.z1);
    gp_Pnt p1(l.x2, l.y2, l.z2);
    cout<<"added line"<<endl;
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(p0, p1).Edge();
    ls_.Append(e);
  }
}

void DXFReader::addPolyline(const DL_PolylineData &pl)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    lp_.reset();
  }
}

void DXFReader::addVertex(const DL_VertexData &pv)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    gp_Pnt p(pv.x, pv.y, pv.z);
    if (lp_.get())
    {
      cout<<"added line"<<endl;
      TopoDS_Edge e=BRepBuilderAPI_MakeEdge(*lp_, p).Edge();
      ls_.Append(e);
    }
    lp_.reset(new gp_Pnt(p));
  }
}

void DXFReader::addSpline(const DL_SplineData& sp)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addSpline"<<endl;
    spl_deg_=sp.degree;
    spl_nctrl_=sp.nControl;
    spl_nknot_=sp.nKnots;
    splp_.clear();
    splk_.clear();
  }
}

void DXFReader::addKnot(const DL_KnotData& kd)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addknot"<<endl;
    splk_.push_back(kd.k);
    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_)) buildSpline();
  }
}

void DXFReader::addControlPoint(const DL_ControlPointData& cp)
{
  DL_Attributes attr=getAttributes();
  if (attr.getLayer()==layername_)
  {
    cout<<"addctrlp"<<endl;
    splp_.push_back(gp_Pnt(cp.x, cp.y, cp.z));
    if ((splk_.size()==spl_nknot_) && (splp_.size()==spl_nctrl_)) buildSpline();
  }
}

void DXFReader::buildSpline()
{
  cout<<"building spline"<<endl;

  int np=splp_.size(), nk=splk_.size();
  TColgp_Array1OfPnt poles(1, np);
  TColStd_Array1OfReal weights(1, np);
  int i;
  for(i=0;i<np;++i)
  {
    cout<<i<<" : "<<splp_[i].X()<<" "<<splp_[i].Y()<<" "<<splp_[i].Z()<<endl;
    poles.SetValue(i+1, splp_[i]);
  }
  for(i=0;i<np;++i)
  {
    weights.SetValue(i+1, 1);
  }

  int degree = spl_deg_;
  int nkr=spl_nknot_; //+degree+1-2*degree;
  
  std::vector<int> mult;
  std::vector<double> u;
  u.push_back(splk_[0]);
  mult.push_back(1);
  for (int i=1; i<splk_.size(); i++)
  {
    double cu=splk_[i];
    double lu=splk_[i-1];
    if (fabs(cu-lu)<1e-10)
    {
      mult.back()++;
    }
    else
    {
      u.push_back(cu);
      mult.push_back(1);
    }
  }

  TColStd_Array1OfReal UKnots(1, u.size());
  TColStd_Array1OfInteger UMult(1, mult.size());
  for(i=0;i<u.size();++i)
  {
    cout<<i<<" : "<<u[i]<<endl;
    UKnots.SetValue(i+1, u[i]);
  }
  for(i=0;i<mult.size();++i)
  {
    cout<<i<<" : "<<mult[i]<<endl;
    UMult.SetValue(i+1, mult[i]);
  }
//   UMult.SetValue(1,degree+1);
//   UMult.SetValue(nkr,degree+1);

  Handle_Geom_BSplineCurve c = new Geom_BSplineCurve
      (
          poles,
          weights,
          UKnots,
          UMult,
          degree
      );

      
    TopoDS_Edge e=BRepBuilderAPI_MakeEdge(c).Edge();
    ls_.Append(e);      
}



TopoDS_Wire DXFReader::Wire() const
{
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ls_);
  return wb.Wire();
}

TopoDS_Shape Sketch::makeSketch(const Datum& pl, const boost::filesystem::path& filename, const std::string& layername)
{
  if (!pl.providesPlanarReference())
    throw insight::Exception("Sketch: Planar reference required!");
  
  TopoDS_Wire w = DXFReader(filename, layername).Wire();
  gp_Trsf tr;
  gp_Ax3 ax=pl;
  tr.SetTransformation(ax);
  
  BRepBuilderAPI_Transform btr(w, tr.Inverted(), true);

  return BRepBuilderAPI_MakeFace(gp_Pln(ax), TopoDS::Wire(btr.Shape())).Shape();
}

Sketch::Sketch(const Datum& pl, const boost::filesystem::path& filename, const std::string& layername)
: SolidModel(makeSketch(pl, filename, layername))
{
}

Sketch::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

}
}