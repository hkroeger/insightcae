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

#ifndef INSIGHT_CAD_BOOLEANINTERSECTION_H
#define INSIGHT_CAD_BOOLEANINTERSECTION_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight
{
namespace cad
{


class BooleanIntersection
: public DerivedFeature
{
  FeaturePtr m1_, m2_;
  DatumPtr m2pl_;
  
  BooleanIntersection(FeaturePtr m1, FeaturePtr m2);
  BooleanIntersection(FeaturePtr m1, DatumPtr m2pl);

public:
  declareType("BooleanIntersection");
  BooleanIntersection();
  
  static FeaturePtr create(FeaturePtr m1, FeaturePtr m2);
  static FeaturePtr create_plane(FeaturePtr m1, DatumPtr m2pl);
  
  virtual void build();
  
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  virtual FeatureCmdInfoList ruleDocumentation() const;
  
  void operator=(const BooleanIntersection& o);
};

FeaturePtr operator&(FeaturePtr m1, FeaturePtr m2);

}
}

#endif // INSIGHT_CAD_BOOLEANINTERSECTION_H
