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
  boost::variant<FeaturePtr,DatumPtr> m2_;
  
  BooleanIntersection(const BooleanIntersection&o, TreeCloneMap& tcm);
  BooleanIntersection(ConstFeaturePtr m1, FeaturePtr m2);
  BooleanIntersection(ConstFeaturePtr m1, DatumPtr m2pl);

  size_t calcHash() const override;
  void build() override;

public:
  declareType("BooleanIntersection");
#ifndef SWIG
  DEPENDS_W_BASE(DerivedFeature, (m2_));
#endif

  CREATE_FUNCTION(BooleanIntersection);
  CLONEABLE(BooleanIntersection);
//  static FeaturePtr create(FeaturePtr m1, FeaturePtr m2);
//  static FeaturePtr create_plane(FeaturePtr m1, DatumPtr m2pl);
  
  
  static void insertrule(parser::ISCADParser& ruleset);
  static FeatureCmdInfoList ruleDocumentation();
  
  void operator=(const BooleanIntersection& o);
};

std::shared_ptr<BooleanIntersection> operator&(FeaturePtr m1, FeaturePtr m2);

}
}

#endif // INSIGHT_CAD_BOOLEANINTERSECTION_H
