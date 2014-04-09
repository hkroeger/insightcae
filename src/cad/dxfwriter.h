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
  void writeLine_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeCircle(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeCircle_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeEllipse(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeDiscrete(const BRepAdaptor_Curve& c, const std::string& layer);
  void writeDiscrete_HatchLoop(const BRepAdaptor_Curve& c, const std::string& layer);
  
  void writeShapeEdges(const TopoDS_Shape& s, std::string layer="0");
  void writeSection(const TopoDS_Shape& s, std::string layer="0");
  
  static void writeViews(const boost::filesystem::path& file, const SolidModel::Views& views);
};
}
}

#endif // INSIGHT_CAD_DXFWRITER_H
