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

#include "occshapeutils.h"
#include "cadexception.h"
#include "base/exception.h"

#include "cadfeatures/importsolidmodel.h"
#include <algorithm>

namespace insight {
namespace cad {


bool isShapeEmpty(const TopoDS_Shape& shape)
{

    if (shape.IsNull())
        return true;

    // Leeres Compound?
    if (shape.ShapeType() == TopAbs_COMPOUND)
    {
        TopoDS_Iterator it(shape);
        if (!it.More())
            return true;
    }

    bool feats=false;
    // Enthält irgendeine Geometrie (Vertex, Edge, Face, ...)?
    for (int shapeType = TopAbs_VERTEX; shapeType <= TopAbs_FACE; ++shapeType)
    {
        TopExp_Explorer exp(shape, static_cast<TopAbs_ShapeEnum>(shapeType));
        if (exp.More())
            feats=true;
    }

    return feats;
}


TopoDS_Face asSingleFace(const TopoDS_Shape &shape)
{
    insight::CurrentExceptionContext exc("converting shape into single face");

    TopExp_Explorer ex(shape, TopAbs_FACE);
    insight::assertion(ex.More(), "shape does not contain any face");

    auto f=TopoDS::Face(ex.Current());
    ex.Next();
    if (ex.More())
    {
        throw insight::CADException(
            {
                { "geometry", cad::Import::create(shape) }
            },
            "Shape contains more than a single face!"
        );
    }

    return f;
}


std::vector<arma::mat> orderedCornerPoints(const TopoDS_Shape &s)
{
    auto f=asSingleFace(s);

    std::vector<arma::mat> pts;
    auto w=BRepTools::OuterWire(f);
    for ( BRepTools_WireExplorer wex(w, f);
          wex.More(); wex.Next() )
    {
        auto p = BRep_Tool::Pnt(wex.CurrentVertex());
        pts.push_back(vec3(p));
    }

    if (f.Orientation()==TopAbs_REVERSED)
        std::reverse(pts.begin(), pts.end());

    return pts;
}

}
}
