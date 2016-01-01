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

#ifndef INSIGHT_CAD_CIRCULARPATTERN_H
#define INSIGHT_CAD_CIRCULARPATTERN_H

#include "cadfeature.h"

namespace insight {
namespace cad {

class CircularPattern
: public Feature
{
//   TopoDS_Shape makePattern(const SolidModel& m1, const arma::mat& p0, const arma::mat& axis, int n, bool center=false);
  FeaturePtr m1_;
  VectorPtr p0_;
  VectorPtr axis_;
  ScalarPtr n_; 
  bool center_;
  
public:
  declareType("CircularPattern");
  CircularPattern(const NoParameters& nop = NoParameters());
  CircularPattern(FeaturePtr m1, VectorPtr p0, VectorPtr axis, ScalarPtr n, bool center=false);
    
  virtual void build();
  
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


}
}

#endif // INSIGHT_CAD_CIRCULARPATTERN_H
