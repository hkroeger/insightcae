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

#include "occbndbox.h"

namespace insight {
namespace cad {

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, double deflection)
{

    if (deflection>0){
        Bnd_Box box;
#if (OCC_VERSION_MAJOR>=7 && OCC_VERSION_MINOR>=4)
        IMeshTools_Parameters p;
        p.Angle=0.5;
        p.Deflection=deflection;
        p.Relative=false;
        BRepMesh_IncrementalMesh  m(shape, p);
#else
#if (OCC_VERSION_MAJOR>=7)
        BRepMesh_FastDiscret::Parameters p;
        p.Angle=0.5;
        p.Deflection=deflection;
        p.Relative=false;
        BRepMesh_FastDiscret m(box, p);
#else
        BRepMesh_FastDiscret m(deflection, 0.5, box, true, false, false, false);
#endif
        m.Perform(shape);
#endif
        //    BRepMesh_IncrementalMesh Inc(shape, deflection);
    }


    Bnd_Box bounds;
    BRepBndLib::Add(shape, bounds);
    return bounds;
}

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, gp_Pnt& bbMin, gp_Pnt& bbMax, double deflection )
{
    Bnd_Box bounds;
    double ext[6];
    bounds = getBoundingBox(shape, deflection);
    bounds.Get(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);

    bbMin.SetCoord(ext[0],ext[1],ext[2]);
    bbMax.SetCoord(ext[3],ext[4],ext[5]);

    return bounds;
}



arma::mat calcBndBox(TopoDS_Shape s)
{
    Bnd_Box boundingBox;
    BRepBndLib::Add(s, boundingBox);

    arma::mat x=arma::zeros(3,2);
    if (!boundingBox.IsVoid())
    {
        double g=boundingBox.GetGap();
        double x0, x1, y0, y1, z0, z1;

        boundingBox.Get
            (
                x0, y0, z0,
                x1, y1, z1
                );

        x = ArmaMatCmpts{
            { x0, x1},
            { y0, y1},
            { z0, z1}
        };

        x.col(0)+=g;
        x.col(1)-=g;
    }

    return x;
}

}
}
