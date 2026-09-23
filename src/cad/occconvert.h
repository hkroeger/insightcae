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

#ifndef INSIGHT_OCCCONVERT_H
#define INSIGHT_OCCCONVERT_H

#include "base/linearalgebra.h"
#include "occinclude.h"

namespace insight {
namespace cad {

inline gp_Pnt to_Pnt(const arma::mat& xyz)
{
    return gp_Pnt(xyz(0), xyz(1), xyz(2));
}

inline gp_Vec to_Vec(const arma::mat& xyz)
{
    return gp_Vec(xyz(0), xyz(1), xyz(2));
}

inline gp_Dir to_Dir(const arma::mat& xyz)
{
    return gp_Dir(xyz(0), xyz(1), xyz(2));
}

}
}

#endif // INSIGHT_OCCCONVERT_H
