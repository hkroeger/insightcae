/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2014  hannes <email>
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

#ifndef INSIGHT_CAD_DXFWRITER_H
#define INSIGHT_CAD_DXFWRITER_H

#include "boost/filesystem.hpp"
#include "dxflib/dl_dxf.h"
#include "boost/assign.hpp"
#include "occinclude.h"
#include "solidmodel.h"

namespace insight {
namespace cad {

typedef boost::tuple<std::string, DL_Attributes, bool> LayerDefinition;

std::vector<gp_Pnt> discretizeBSpline(const BRepAdaptor_Curve& c);


struct hatchLoopWriter
{
  virtual void write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const =0;
  virtual int nsegments() const =0;
  virtual void alignStartWith(const gp_Pnt& p) =0;
  virtual gp_Pnt& start() =0;
  virtual gp_Pnt& end() =0;
};

struct writerLine_HatchLoop
:public hatchLoopWriter
{
  gp_Pnt p0, p1;
  writerLine_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer, bool reverse=false);
  virtual void write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const;

  virtual int nsegments() const { return 1; }
  virtual void alignStartWith(const gp_Pnt& p) { p0=p; }
  virtual gp_Pnt& start() { return p0; }
  virtual gp_Pnt& end() { return p1; }
};

struct writerCircle_HatchLoop
:public hatchLoopWriter
{
  gp_Pnt p;
  double r, start_angle, end_angle;
  writerCircle_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer);
  virtual void write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const;
  
  virtual int nsegments() const { return 1; }
  virtual void alignStartWith(const gp_Pnt& p) {};
#warning garbage!
  virtual gp_Pnt& start() { return p; }
#warning garbage!
  virtual gp_Pnt& end() { return p; }
};

struct writerDiscrete_HatchLoop
:public hatchLoopWriter
{
  std::vector<gp_Pnt> pts;
  
  writerDiscrete_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer, bool reverse=false);
  virtual void write(DL_Dxf& dxf, std::auto_ptr<DL_WriterA>& dw) const;
  
  virtual int nsegments() const { return pts.size()-1; }
  virtual void alignStartWith(const gp_Pnt& p) { pts[0]=p; };
  virtual gp_Pnt& start() { return pts[0]; }
  virtual gp_Pnt& end() { return pts.back(); }
};

class HatchGenerator
{
public:
  struct HatchData
  {
    double scale, angle;
    HatchData(double scale, double angle);
  };
  
protected:
  int curidx_;
  static std::vector<HatchData> hatches_;
  
public:
  HatchGenerator();
  
  DL_HatchData generate();
};

class DXFWriter
{
protected:
  DL_Dxf dxf_;
  DL_Codes::version exportVersion_;
  std::auto_ptr<DL_WriterA> dw_;
  
public:
  DXFWriter
  (
    const boost::filesystem::path& file,
    const std::vector<LayerDefinition>& layers = std::vector<LayerDefinition>()
  );
  ~DXFWriter();

  void writeLine(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeCircle(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeEllipse(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeDiscrete(const BRepAdaptor_Curve& c, const std::string& layer);
  
  void writeShapeEdges(const TopoDS_Shape& s, std::string layer="0");
  void writeSection(const TopoDS_Shape& s, HatchGenerator& hgen, std::string layer="0");
  
  static void writeViews(const boost::filesystem::path& file, const SolidModel::Views& views);
};
}
}

#endif // INSIGHT_CAD_DXFWRITER_H
