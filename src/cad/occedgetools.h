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

#ifndef INSIGHT_OCCEDGETOOLS_H
#define INSIGHT_OCCEDGETOOLS_H

#include "base/linearalgebra.h"
#include "occinclude.h"

namespace insight {
namespace cad {

double edgeLength(const TopoDS_Edge& e);

std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, double approxSegmentLength, double* actualSegmentLength=NULL, int minSegments=1);
std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, int nSegments);
std::vector<double> resampleEdge (const TopoDS_Edge& edge, const std::vector<double> lIn, int c0Res=-1);

gp_Pnt edgeAt(const TopoDS_Edge& edge, double t);

arma::mat edgeTangent(TopoDS_Shape edge, const arma::mat& pt);

}
}

#endif // INSIGHT_OCCEDGETOOLS_H
