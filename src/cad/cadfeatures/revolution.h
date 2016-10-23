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

#ifndef INSIGHT_CAD_REVOLUTION_H
#define INSIGHT_CAD_REVOLUTION_H

#include "cadfeature.h"

namespace insight {
namespace cad {


class Revolution
: public Feature
{
  FeaturePtr sk_;
  VectorPtr p0_;
  VectorPtr axis_;
  ScalarPtr angle_;
  bool centered_;
  
  Revolution(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr angle, bool centered=false);
  
public:
  declareType("Revolution");
  Revolution(const NoParameters& nop = NoParameters());

  static FeaturePtr create(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr angle, bool centered=false);
  
  virtual void build();
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


}
}

#endif // INSIGHT_CAD_REVOLUTION_H
