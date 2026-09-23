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

#include "occfacetools.h"

#include <algorithm>

namespace insight {
namespace cad {


gp_Pnt2d faceUV(const TopoDS_Face& f, const TopoDS_Vertex& v)
{
    return faceUV(f, BRep_Tool::Pnt(v));
}

gp_Pnt2d faceUV(const TopoDS_Face& f, const gp_Pnt& p)
{
    return faceUV(BRep_Tool::Surface(f), p);
}

gp_Pnt2d faceUV(const Handle_Geom_Surface& f, const gp_Pnt& p)
{
    ShapeAnalysis_Surface sas(f);
    auto uv = sas.ValueOfUV(p, 1e-7);
    double u1, u2, v1, v2;
    f->Bounds(u1, u2, v1, v2);
    return gp_Pnt2d(
        std::min(u2, std::max(u1, uv.X())),
        std::min(v2, std::max(v1, uv.Y()))
        );
}

std::vector<gp_XY> faceUV
    (
        const TopoDS_Face& f,
        std::vector<gp_Pnt>::const_iterator begin,
        std::vector<gp_Pnt>::const_iterator end
        )
{
    std::vector<gp_XY> res;
    for (std::vector<gp_Pnt>::const_iterator i=begin; i!=end; i++)
    {
        res.push_back(faceUV(f, *i).XY());
    }
    return res;
}

gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt& v)
{
    return faceNormal(f, faceUV(f, v));
}

gp_Vec faceNormal(const TopoDS_Face& f, const TopoDS_Vertex& v)
{
    return faceNormal(f, faceUV(f, v));
}

gp_Vec faceNormal(const TopoDS_Face& f, const gp_Pnt2d& p2)
{
    gp_Vec n=GeomLProp_SLProps
               (
                   BRep_Tool::Surface(f),
                   p2.X(), p2.Y(),
                   1, Precision::Confusion()
                   ).Normal();
    n.Normalize();
    if (f.Orientation()==TopAbs_REVERSED){
        n.Multiply(-1.);
    }
    return n;
}


gp_Pnt faceAt(const TopoDS_Face& face, gp_Pnt2d p2d )
{
    return faceAt(face, p2d.X(),p2d.Y());
}

gp_Pnt faceAt(const TopoDS_Face& face, double u, double v )
{
    Handle_Geom_Surface surf_=BRep_Tool::Surface(face);
    //  return surf_->Value(min(1., max(0., u)), min(1., max(0., v)));
    return surf_->Value(u, v);
}

std::vector<gp_Pnt> faceAt
    (
        const TopoDS_Face& face,
        std::vector<gp_XY>::const_iterator begin,
        std::vector<gp_XY>::const_iterator end
        )
{
    std::vector<gp_Pnt> res;
    for (std::vector<gp_XY>::const_iterator i=begin; i!=end; i++)
    {
        res.push_back(faceAt(face, i->X(), i->Y()));
    }
    return res;
}


}
}
