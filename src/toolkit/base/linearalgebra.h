/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2013  hannes <email>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef INSIGHT_LINEARALGEBRA_H
#define INSIGHT_LINEARALGEBRA_H

#include <armadillo>

namespace insight 
{

arma::mat vec3(double x, double y, double z);
arma::mat tensor3(
  double xx, double xy, double xz,
  double yx, double yy, double yz,
  double zx, double zy, double zz
);
arma::mat vec2(double x, double y);

template<class T>
arma::mat Tensor(const T& t)
{
  arma::mat rt;
  rt << t.XX() << t.XY() << t.XZ() << arma::endr
     << t.YX() << t.YY() << t.YZ() << arma::endr
     << t.ZX() << t.ZY() << t.ZZ() << arma::endr;
  return rt;
}

template<class T>
arma::mat tensor(const T& t)
{
  arma::mat rt;
  rt << t.xx() << t.xy() << t.xz() << arma::endr
     << t.yx() << t.yy() << t.yz() << arma::endr
     << t.zx() << t.zx() << t.zz() << arma::endr;
  return rt;
}

arma::mat rotMatrix( double theta, arma::mat u=vec3(0,0,1) );

std::string toStr(const arma::mat& v3);

/**
 * Fits c_j in
 *  c_j*x_ij approx y_i
 * add a column with all values "1" to x_ij to include constant offset!
 * 
 * y: column vector with values to fit
 * x: matrix with x-values
 */
arma::mat linearRegression(const arma::mat& y, const arma::mat& x);

arma::mat polynomialRegression(const arma::mat& y, const arma::mat& x, int maxorder, int minorder=0);

}

#endif // INSIGHT_LINEARALGEBRA_H
