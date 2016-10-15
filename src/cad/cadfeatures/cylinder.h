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
 */

#ifndef INSIGHT_CAD_CYLINDER_H
#define INSIGHT_CAD_CYLINDER_H

#include "cadfeature.h"

namespace insight {
namespace cad {


class Cylinder
: public SingleVolumeFeature
{
  bool p2isAxis_;
  VectorPtr p1_;
  VectorPtr p2_;
  ScalarPtr D_;
  
  ScalarPtr Di_;
  
  bool centered_;
  
  Cylinder(VectorPtr p1, VectorPtr p2, ScalarPtr D, bool p2isAxis, bool centered);
  Cylinder(VectorPtr p1, VectorPtr p2, ScalarPtr Da, ScalarPtr Di, bool p2isAxis, bool centered);

public:
  declareType("Cylinder");
  Cylinder(const NoParameters& nop = NoParameters());
  
  static FeaturePtr create(VectorPtr p1, VectorPtr p2, ScalarPtr D, bool p2isAxis, bool centered);
  static FeaturePtr create(VectorPtr p1, VectorPtr p2, ScalarPtr Da, ScalarPtr Di, bool p2isAxis, bool centered);
  
  virtual void build();
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


}
}

#endif // INSIGHT_CAD_CYLINDER_H
