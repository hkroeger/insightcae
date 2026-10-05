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

#ifndef INSIGHT_CAD_DIAMETERPP_H
#define INSIGHT_CAD_DIAMETERPP_H

#include "cadtypes.h"
#include "cadpostprocaction.h"



namespace insight {
namespace cad {




/**
 * @brief The DiameterMeasurement class
 * measures the diameter of a circle, which is defined either by
 * - a circular edge or
 * - a center point, the normal vector and a point on the curve or
 * - three points on the curve
 */
class DiameterMeasurement
: public PostprocAction
{
public:
  enum Mode { CircularEdge, CenterNormalPoint, ThreePoints };

private:
  Mode mode_;

  FeatureSetPtr edge_;

  /**
   * CenterNormalPoint: center point, normal vector, point on curve
   * ThreePoints: three points on curve
   */
  VectorPtr v1_, v2_, v3_;

  arma::mat center_, normal_;

  /**
   * in-plane direction of the dimension line
   */
  arma::mat refDir_;

  double diameter_;

  size_t calcHash() const override;

  DiameterMeasurement(FeatureSetPtr edge);
  DiameterMeasurement(Mode mode, VectorPtr v1, VectorPtr v2, VectorPtr v3);

public:
  declareType("ShowDiameter");
  CREATE_FUNCTION(DiameterMeasurement);

  void build() override;

  void write(std::ostream& console) const override;

  double diameter() const;
  arma::mat center() const;
  arma::mat normal() const;

  std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const override;

  static void insertrule(parser::ISCADParser& ruleset);
};




}
}

#endif // INSIGHT_CAD_DIAMETERPP_H
