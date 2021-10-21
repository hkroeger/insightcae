/*
 * <one line to give the program's name and a brief idea of what it does.>
 * Copyright (C) 2015  hannes <email>
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
 *
 */

#ifndef INSIGHT_OCCTOOLS_H
#define INSIGHT_OCCTOOLS_H

#include "occinclude.h"
#include "vtkSmartPointer.h"
#include "vtkTransform.h"

namespace insight {
namespace cad {


gp_Trsf OFtransformToOCC(const arma::mat& translate, const arma::mat& rollPitchYaw, double scale);

class OCCtransformToOF
{
  arma::mat translate_;
  arma::mat rollPitchYaw_;
  double scale_;

public:
  OCCtransformToOF(const gp_Trsf& t);
  inline const arma::mat& translate() const { return translate_; }
  /**
   * @brief rollPitchYaw
   * @return Euler angles in degrees.
   */
  inline const arma::mat& rollPitchYaw() const { return rollPitchYaw_; }
  inline double scale() const { return scale_; }
};




class OCCtransformToVTK
    : public OCCtransformToOF
{
public:
  OCCtransformToVTK(const gp_Trsf& t);

  vtkSmartPointer<vtkTransform> operator()() const;
};



}
}

#endif // INSIGHT_OCCTOOLS_H
