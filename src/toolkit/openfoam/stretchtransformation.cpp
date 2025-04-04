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

#include "stretchtransformation.h"

#include <cmath>
#include <iostream>

using namespace std;

namespace insight {
  

stretchTransformation::stretchTransformation
(
  double compression,
  double x1, double x2,
  double xw
)
: s_((x1+compression)/x1), x1_(x1), x2_(x2), x1s_(x1_+compression), xw_(xw)
{
  x2s_ = 0.5*(s_*x2_ +x2_ +2.*x1s_ -(1.+s_)*x1_);

  std::cout<<"s="<<s_<<", x1="<<x1_<<", x2="<<x2_<<", x1s="<<x1s_<<", x2s="<<x2s_<<", xw="<<xw_<<endl;
}


arma::mat stretchTransformation::toBox(const arma::mat& p) const
{
  arma::mat xs=p;

  double x=p(2) - xw_;
  double s=copysign(1.0, x); //SIGN(dz);
  double newx;
  x=fabs(x);
  if (x<x1_)
  {
    newx = s_*x;
  }
  else if ((x>=x1_) && (x<x2_))
  {
    newx = x1s_ + ( (x-x1_) *  ((s_-1.)*x +x1_ +s_*x1_ -2.*s_*x2_) ) / ( 2.*(x1_-x2_) );
  }
  else
  {
    newx = x - 0.5*(s_+1.)*x1_ +x1s_ +0.5*(s_-1.)*x2_;
  }
  newx *= s;
  xs(2) = newx + xw_;

  return xs;
}



arma::mat stretchTransformation::toWedge(const arma::mat& ps) const
{
  arma::mat p=ps;

  double y=ps(2) - xw_;
  double s=copysign(1.0, y); //SIGN(dz);
  double x;
  y=fabs(y);
  if ( y<x1s_ )
  {
    x=y/s_;
  }
  else if ( (y>=x1s_) && (y<x2s_) )
  {
      double radi=
          (x1_-x2_)
            *
          (
            2.*(x1s_-y)
            +
            s_*( s_*x1_ - 2.*x1s_ - s_*x2_ + 2.*y)
          );

      double nom = -(
        x1_
        - s_*x2_
        + sqrt(radi)
        );

      double denom = s_-1.;

      if (fabs(s_-1.)<SMALL)
      {
          x = y -x1s_ +x1_;
      }
      else
      {
          x = nom / denom;
      }
  }
  else
  {
      x = 0.5*(x1_ +s_*x1_ -2.*x1s_ +x2_ -s_*x2_) + y;
  }

  p(2) = s*x + xw_;

  return p;
}

}
