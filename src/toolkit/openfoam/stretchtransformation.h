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
 *
 */

#ifndef INSIGHT_STRETCHTRANSFORMATION_H
#define INSIGHT_STRETCHTRANSFORMATION_H

#include "base/linearalgebra.h"

namespace insight {
  
//#define TRSFT1
//#define TRSFT2
#define TRSFT3

class stretchTransformation
{
protected:

  double s_, x1_, x2_, x1s_, x2s_;
  double xw_;

public:
  /**
   * @param compression The compression measure: absolute height increase of zone within x1 into box space
   * @param x1 height of the zone with constant maximum compression
   * @param x2 height coordinate of end of transition zone
   * @param xw location of waterline (center of compressed zone)
   */
  stretchTransformation(double compression,
		     double x1, double x2,
		     double xw=0.0);
  
  /**
   * Transform into box (stretch geometry coordinates)
   * @param p point coordinates
   */
  arma::mat toBox(const arma::mat& p) const;
  
  /**
   * Transform from stretched CS back into original CS
   * @param p point coordinates
   */
  arma::mat toWedge(const arma::mat& p) const;
  
  inline double addHeight(const arma::mat&p) const
  {
    return toBox(p)(2)-p(2);
  }
};

}

#endif // INSIGHT_STRETCHTRANSFORMATION_H
