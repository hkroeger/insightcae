 
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

#include "base/cppextensions.h"
#include "cadtypes.h"
#include "cadpostprocaction.h"



namespace insight {
namespace cad {




class Distance
: public PostprocAction
{


  size_t calcHash() const override;

public:
  VectorPtr p1_, p2_;
  VectorPtr distanceAlong_;

  double distance_;

  arma::mat measureDirection() const;

protected:
  double calcDistance() const;

public:
  declareType("ShowDistance");

  Distance(VectorPtr p1, VectorPtr p2, VectorPtr distanceAlong=VectorPtr());

  void build() override;

  void write(std::ostream&) const override;
//  virtual Handle_AIS_InteractiveObject createAISRepr() const;

  void operator=(const Distance& other);

  /**
   * @brief dimLineOffset
   * @return
   * offset of dimension line from connection between points.
   */
  virtual arma::mat dimLineOffset() const;
  virtual double relativeArrowSize() const;

  double distance() const;
  arma::mat symbolLocation() const;

  std::vector<vtkSmartPointer<vtkProp> > createVTKRepr(bool displayCoords) const;
  std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const override;
};





}
}

#endif // INSIGHT_CAD_DISTANCEPP_H
