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

#ifndef INSIGHT_OCCBNDBOX_H
#define INSIGHT_OCCBNDBOX_H

#include "base/linearalgebra.h"
#include "occinclude.h"

namespace insight {
namespace cad {

arma::mat calcBndBox(TopoDS_Shape);

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, double deflection=-1);
Bnd_Box getBoundingBox(const TopoDS_Shape& shape, gp_Pnt& bbMin, gp_Pnt& bbMax, double deflection=-1 );

}
}

#endif // INSIGHT_OCCBNDBOX_H
