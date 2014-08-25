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
  
  BRepBuilderAPI_Transform btr(w, tr);

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