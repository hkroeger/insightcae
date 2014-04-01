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

#ifndef INSIGHT_CAD_SKETCH_H
#define INSIGHT_CAD_SKETCH_H

#include "base/linearalgebra.h"
#include "occinclude.h"
#include "datum.h"

#include "dxflib/dl_creationadapter.h"
#include "dxflib/dl_dxf.h"

#include "boost/filesystem.hpp"

namespace insight {
namespace cad {

class DXFReader
: public DL_CreationAdapter
{
protected:
  TopTools_ListOfShape ls_;
  std::auto_ptr<gp_Pnt> lp_;
  
public:
  DXFReader(const boost::filesystem::path& filename);
  virtual void addLine(const DL_LineData &);
  virtual void addPolyline(const DL_PolylineData &);
  virtual void addVertex(const DL_VertexData &);
  TopoDS_Wire Wire() const;
};

class Sketch
{
protected:
  TopoDS_Face face_;
  
public:
  Sketch(const DatumPlane& pl, const boost::filesystem::path& filename);
  operator const TopoDS_Face& () const;
};

}
}

#endif // INSIGHT_CAD_SKETCH_H
