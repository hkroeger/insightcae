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

#ifndef INSIGHT_CAD_SKETCH_H
#define INSIGHT_CAD_SKETCH_H

#include "base/linearalgebra.h"
#include "occinclude.h"
#include "datum.h"

#include "dxflib/dl_creationadapter.h"
#include "dxflib/dl_dxf.h"

#include "base/boost_include.h"

namespace insight {
namespace cad {

class DXFReader
: public DL_CreationAdapter
{
protected:
  std::string layername_;
  TopTools_ListOfShape ls_;
  
  struct Polyline
  {
    DXFReader& reader;
    
    Polyline(DXFReader&);
    ~Polyline();
    
    bool closed;
    std::auto_ptr<gp_Pnt> lp, p0;
  };
  
  mutable std::auto_ptr<Polyline> pl_;
  
  int spl_deg_, spl_nknot_, spl_nctrl_;
  std::vector<gp_Pnt> splp_;
  std::vector<double> splk_;
  
public:
  DXFReader(const boost::filesystem::path& filename, const std::string& layername="0");
  virtual ~DXFReader();
  virtual void addArc(const DL_ArcData &);
  virtual void addLine(const DL_LineData &);
  virtual void addPolyline(const DL_PolylineData &);
  virtual void addVertex(const DL_VertexData &);
  
  virtual void addSpline(const DL_SplineData&);
  virtual void addKnot(const DL_KnotData&);
  virtual void addControlPoint(const DL_ControlPointData&);
  void buildSpline();
  
  TopoDS_Wire Wire() const;
};

class Sketch
: public SolidModel
{
//   TopoDS_Shape makeSketch(const Datum& pl, const boost::filesystem::path& filename, const std::string& layername="0");

public:
  Sketch(const Datum& pl, const boost::filesystem::path& filename, const std::string& layername="0");
  operator const TopoDS_Face& () const;
};

}
}

#endif // INSIGHT_CAD_SKETCH_H
