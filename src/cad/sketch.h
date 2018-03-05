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

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadfeature.h"

#include "dxfreader.h"

#include "base/boost_include.h"

namespace insight {
namespace cad {

typedef std::vector<boost::fusion::vector2<std::string, ScalarPtr> > SketchVarList;

class Sketch
: public Feature
{
  DatumPtr pl_;
  boost::filesystem::path fn_;
  std::string ln_;
  SketchVarList vars_;
  double tol_;

  Sketch
  (
    DatumPtr pl, 
    const boost::filesystem::path& filename, 
    const std::string& layername="0", 
    const SketchVarList& vars = SketchVarList(), 
    double tol=Precision::Confusion() 
  );

  virtual size_t calcHash() const;
  virtual void build();

public:
  declareType("Sketch");
  Sketch();
  
  static FeaturePtr create
  (
    DatumPtr pl, 
    const boost::filesystem::path& filename, 
    const std::string& layername="0", 
    const SketchVarList& vars = SketchVarList(), 
    double tol=Precision::Confusion() 
  );
  
  void operator=(const Sketch& o);
  
  
  void executeEditor();
  
//   virtual bool isSingleCloseWire() const;
//   virtual TopoDS_Wire asSingleClosedWire() const;
  virtual bool isSingleFace() const;
  operator const TopoDS_Face& () const;
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  
  const boost::filesystem::path& fn() const { return fn_; }
};

}
}

#endif // INSIGHT_CAD_SKETCH_H
