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

#ifndef INSIGHT_CAD_PROJECTEDPOINT_H
#define INSIGHT_CAD_PROJECTEDPOINT_H

#include "cadparameter.h"

namespace insight {
namespace cad {

    
class ProjectedPoint 
: public insight::cad::Vector
{
  VectorPtr p0_;
  DatumPtr plane_;

  VectorPtr along_;
  
  ProjectedPoint(const ProjectedPoint&o, TreeCloneMap& tcm);

public:
#ifndef SWIG
    DEPENDS((p0_, plane_, along_));
#endif
  CLONEABLE(ProjectedPoint);

  ProjectedPoint(VectorPtr p0, DatumPtr plane);
  ProjectedPoint(VectorPtr p0, DatumPtr plane, VectorPtr along);
  size_t calcHash() const override;
  arma::mat calcValue() const override;
};



class ProjectedPointOnFeature
: public insight::cad::Vector
{
  VectorPtr p0_;
  VectorPtr along_;
  FeaturePtr targ_;

  ProjectedPointOnFeature(const ProjectedPointOnFeature&o, TreeCloneMap& tcm);

public:
#ifndef SWIG
    DEPENDS((p0_, along_, targ_));
#endif
  CLONEABLE(ProjectedPointOnFeature);

  ProjectedPointOnFeature(VectorPtr p0, VectorPtr along, FeaturePtr targ);
  size_t calcHash() const override;
  arma::mat calcValue() const override;
};


}
}

#endif // INSIGHT_CAD_PROJECTEDPOINT_H
