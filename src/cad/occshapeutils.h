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

#ifndef INSIGHT_OCCSHAPEUTILS_H
#define INSIGHT_OCCSHAPEUTILS_H

#include "base/linearalgebra.h"
#include "occinclude.h"
#include "BRepAlgoAPI_BooleanOperation.hxx"
#include "BRepBuilderAPI_Copy.hxx"
#include "TopTools_ListOfShape.hxx"

namespace insight {
namespace cad {

bool isShapeEmpty(const TopoDS_Shape& shape);

std::vector<arma::mat> orderedCornerPoints(const TopoDS_Shape& f);
TopoDS_Face asSingleFace(const TopoDS_Shape& shape);

/**
 * performs the boolean operation op (BRepAlgoAPI_Cut, _Fuse, _Common, default constructed)
 * of the argument and the tool.
 * The operation works on copies of their topology (the geometry is shared):
 * OCC modifies the arguments in place (adds pcurves, increases tolerances) and reads
 * their triangulations. The shapes may be shared with other threads (e.g. features restored
 * from the feature cache), which might triangulate them concurrently.
 */
template<class Op>
void performBoolean(Op& op, const TopoDS_Shape& argument, const TopoDS_Shape& tool)
{
    TopTools_ListOfShape args, tools;
    args.Append(BRepBuilderAPI_Copy(argument, Standard_False).Shape());
    tools.Append(BRepBuilderAPI_Copy(tool, Standard_False).Shape());
    op.SetArguments(args);
    op.SetTools(tools);
    op.Build();
}

}
}

#endif // INSIGHT_OCCSHAPEUTILS_H
