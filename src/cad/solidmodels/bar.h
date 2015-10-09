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

#ifndef INSIGHT_CAD_BAR_H
#define INSIGHT_CAD_BAR_H

#include "solidmodel.h"

namespace insight {
namespace cad {


class Bar
: public SolidModel
{
public:
  declareType("Bar");
  Bar(const NoParameters& nop = NoParameters());
  
  /**
   * crate bar between p0 and p1. Cross section's xsec (single face) y-axis will be aligned with vertical direction vert.
   * bar is elongated at p0 by ext0 and at p1 by ext1, respectively.
   * 
   * @param miterangle0_vert Miter angle of bar end at (elongated) p0 around vertical direction
   * @param miterangle0_hor Miter angle of bar end at (elongated) p0 around horizontal direction
   * @param miterangle1_vert Miter angle of bar end at (elongated) p1 around vertical direction
   * @param miterangle1_hor Miter angle of bar end at (elongated) p1 around horizontal direction
   */
  Bar
  (
    const arma::mat& p0, const arma::mat& p1, 
    const SolidModel& xsec, const arma::mat& vert,
    double ext0=0.0, double ext1=0.0,
    double miterangle0_vert=0.0, double miterangle1_vert=0.0,
    double miterangle0_hor=0.0, double miterangle1_hor=0.0
  );
  virtual void insertrule(parser::ISCADParser& ruleset) const;
};


}
}

#endif // INSIGHT_CAD_BAR_H
