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

#ifndef INSIGHT_OCCFACETOOLS_H
#define INSIGHT_OCCFACETOOLS_H

#include "base/linearalgebra.h"
#include "occinclude.h"

namespace insight {
namespace cad {

gp_Pnt2d faceUV(const TopoDS_Face& f, const TopoDS_Vertex& v);
gp_Pnt2d faceUV(const TopoDS_Face& f, const gp_Pnt& p);
gp_Pnt2d faceUV(const Handle_Geom_Surface& f, const gp_Pnt& p);
std::vector<gp_XY> faceUV
    (
        const TopoDS_Face& f,
        std::vector<gp_Pnt>::const_iterator begin,
        std::vector<gp_Pnt>::const_iterator end
        );

gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt& v);
gp_Vec faceNormal(const TopoDS_Face& f, const TopoDS_Vertex& v);
gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt2d& p2);

gp_Pnt faceAt(const TopoDS_Face& face, gp_Pnt2d p2d);
std::vector<gp_Pnt> faceAt
    (
        const TopoDS_Face& face,
        std::vector<gp_XY>::const_iterator begin,
        std::vector<gp_XY>::const_iterator end
        );
gp_Pnt faceAt(const TopoDS_Face& face, double u, double v );

}
}

#endif // INSIGHT_OCCFACETOOLS_H
