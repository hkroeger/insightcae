/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "sketch.h"
#include "base/exception.h"

namespace insight {
namespace cad {

DXFReader::DXFReader(const boost::filesystem::path& filename)
{
  std::auto_ptr<DL_Dxf> dxf(new DL_Dxf());
  if (!dxf->in(filename.c_str(), this)) 
  {
    std::cerr << filename << " could not be opened." << std::endl;
  }
}

void DXFReader::addLine (const DL_LineData &l)
{
  gp_Pnt p0(l.x1, l.y1, l.z1);
  gp_Pnt p1(l.x2, l.y2, l.z2);
  cout<<"added line"<<endl;
  TopoDS_Edge e=BRepBuilderAPI_MakeEdge(p0, p1).Edge();
  ls_.Append(e);
}

void DXFReader::addPolyline(const DL_PolylineData &pl)
{
  lp_.reset();
}

void DXFReader::addVertex(const DL_VertexData &pv)
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

TopoDS_Wire DXFReader::Wire() const
{
  BRepBuilderAPI_MakeWire wb;
  wb.Add(ls_);
  return wb.Wire();
}

Sketch::Sketch(const DatumPlane& pl, const boost::filesystem::path& filename)
{
  TopoDS_Wire w = DXFReader(filename).Wire();
  gp_Trsf tr;
  gp_Ax3 ax=pl;
  tr.SetTransformation(ax);
  
  BRepBuilderAPI_Transform btr(w, tr);

  face_=TopoDS::Face(BRepBuilderAPI_MakeFace(gp_Pln(ax), TopoDS::Wire(btr.Shape())).Shape());
}

Sketch::operator const TopoDS_Face& () const
{
  return face_;
}

}
}