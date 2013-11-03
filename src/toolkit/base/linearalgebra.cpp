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

#include "linearalgebra.h"

using namespace arma;

namespace insight
{

mat vec3(double x, double y, double z)
{
  mat v;
  v << x <<endr << y << endr << z <<endr;
  return v;
}

mat vec2(double x, double y)
{
  mat v;
  v << x <<endr << y << endr;
  return v;
}

mat rotMatrix( double theta, mat u )
{
    double s=sin(theta);
    double c=cos(theta);
    double ux=u[0];
    double uy=u[1];
    double uz=u[2];
    mat m;
    m << ux*ux+(1-ux*ux)*c << ux*uy*(1-c)-uz*s << ux*uz*(1-c)+uy*s << endr
      << ux*uy*(1-c)+uz*s << uy*uy+(1-uy*uy)*c << uy*uz*(1-c)-ux*s << endr
      << ux*uz*(1-c)-uy*s << uy*uz*(1-c)+ux*s << uz*uz+(1-uz*uz)*c << endr;
    return m;
}

}