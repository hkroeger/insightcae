 
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

#ifndef INSIGHT_CAD_DISTANCEPP_H
#define INSIGHT_CAD_DISTANCEPP_H

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadpostprocaction.h"
#include "constrainedsketchgeometry.h"

namespace insight {
namespace cad {

class Distance
: public PostprocAction
{


  size_t calcHash() const override;

public:
  VectorPtr p1_, p2_;
  double distance_;

public:
  declareType("Distance");

  Distance(VectorPtr p1, VectorPtr p2);

  void build() override;

  void write(std::ostream&) const override;
//  virtual Handle_AIS_InteractiveObject createAISRepr() const;

};



class DistanceConstraint
: public Distance,
  public ConstrainedSketchEntity
{
    ScalarPtr targetValue_;

    size_t calcHash() const override;

public:
    declareType("DistanceConstraint");

    DistanceConstraint(VectorPtr p1, VectorPtr p2, ScalarPtr targetValue);

    ScalarPtr targetValue();

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
};

}
}

#endif // INSIGHT_CAD_DISTANCEPP_H
