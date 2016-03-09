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

#ifndef INSIGHT_CAD_TRANSFORM_H
#define INSIGHT_CAD_TRANSFORM_H

#include "cadparameters.h"
#include "derivedfeature.h"

namespace insight 
{
namespace cad 
{


class Transform
: public DerivedFeature
{
//   TopoDS_Shape makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale=1.0);
//   TopoDS_Shape makeTransform(const SolidModel& m1, const gp_Trsf& trsf);
  
  FeaturePtr m1_;
  VectorPtr trans_;
  VectorPtr rot_;
  ScalarPtr sf_;
  
  boost::shared_ptr<gp_Trsf> trsf_;
  
public:
  declareType("Transform");
  
  Transform(const NoParameters& nop = NoParameters());
  Transform(FeaturePtr m1, VectorPtr trans, VectorPtr rot, ScalarPtr sf);
  Transform(FeaturePtr m1, VectorPtr trans);
  Transform(FeaturePtr m1, ScalarPtr scale);
  Transform(FeaturePtr m1, const gp_Trsf& trsf);
  
  virtual void build();
  
  virtual void insertrule(parser::ISCADParser& ruleset) const;
  
  virtual bool isRelocationFeature() const { return true; }
};

}
}

#endif // INSIGHT_CAD_TRANSFORM_H
