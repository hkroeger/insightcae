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

#include "occedgetools.h"
#include "occconvert.h"

#include <algorithm>
#include <stdexcept>

namespace insight {
namespace cad {


double edgeLength(const TopoDS_Edge& e)
{
    GProp_GProps gpr;
    double l = 0.;
    if (!e.IsNull() && !BRep_Tool::Degenerated(e)) {
        BRepGProp::LinearProperties(e, gpr);
        l = gpr.Mass();
    }
    return l;
}


std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, double approxSegmentLength, double* actualSegmentLength, int minSegments)
{
    double length=edgeLength(edge);
    int nSegments;

    if (approxSegmentLength > 0){

        nSegments=ceil(length/approxSegmentLength);
        nSegments = std::max(minSegments, nSegments);
    } else {
        throw insight::Exception("unable to split edge using a negative segment length");
    }

    if (actualSegmentLength){
        *actualSegmentLength = length/(double)(nSegments);
    }
    return resampleEdgeUniform(edge, nSegments);
}

std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, int nSegments)
{
    //  BRepAdaptor_Curve adap(edge);
    //  nSegments = max(1, nSegments);
    //  GCPnts_UniformAbscissa abscissa(adap, nSegments+1, 0.001);
    //  if (!abscissa.IsDone())
    //    throw PGError("Uniform discretization failed");
    //  std::vector<gp_Pnt> tValues;
    //  for (int i=1;i<=nSegments+1;i++){
    //    //tValues.push_back(abscissa.Parameter(i));
    //    gp_Pnt p;
    //    BOPTools_Tools::PointOnEdge(edge, abscissa.Parameter(i), p);
    //    tValues.push_back(p);
    //  }
    //  return tValues;
    std::vector<gp_Pnt> points;
    std::vector<double> tValues, uValues;

    for (int a=0; a<=nSegments; a++){
        double frac = (double)a/double(nSegments);
        tValues.push_back(frac);
    }
    uValues = resampleEdge(edge, tValues, 2.*nSegments);

    for (int a=0; a<(int)uValues.size(); a++){
        points.push_back(edgeAt(edge, uValues[a]));
    }
    return points;

}

/**
 * Returns a list of u-Values, where a u-Value corresponds to a lIn value. The sub-edge
 * 0..u has a length of lIn*length(c0).
 * To calculate the length, the edge c0 is Split up into c0Res sub-edges.
 */
std::vector<double> resampleEdge (const TopoDS_Edge& edge, const std::vector<double> lIn, int c0Res)
{
    double s;//, s0, s1;
    gp_Pnt p0, p1;
    //Handle_Geom_Curve c0 = BRep_Tool::Curve(edge, s0, s1);
    std::vector<double> uOut;
    if (c0Res < 1){
        c0Res = lIn.size()*2;
    }
    c0Res = std::max(1,c0Res);
    double l[c0Res+1], L=0;

    l[0] = 0.;
    p0 = edgeAt(edge, 0.); //c0->Value(s0);
    for(int i=1; i<=c0Res;i++){
        s = (double)(i)/(double)c0Res;
        p1 = edgeAt(edge, s); //c0->Value(s);
        L += p1.Coord().Subtracted(p0.Coord()).Modulus();
        l[i] = L;
        p0 = p1;
    }
    for(int i=1; i<=c0Res;i++){
        l[i] /= L;
    }

    for (unsigned int i=0; i<lIn.size(); i++){
        double li = lIn[i], u;
        int j;
        for (j=0; l[j+1]<li && j<c0Res; j++);
        u = (li-l[j])/(l[j+1]-l[j])*1./(double)c0Res + (double)j/(double)c0Res;
        uOut.push_back(u);
    }

    return uOut;
}


gp_Pnt edgeAt(const TopoDS_Edge& edge, double t)
{
    //  TopTools_IndexedMapOfShape vertexList;
    //  TopExp::MapShapes(edge, TopAbs_VERTEX, vertexList);
    //  if (vertexList.Extent()>0){

    double start, end;
    Handle_Geom_Curve c0 = BRep_Tool::Curve(edge, start, end);

    if (edgeLength(edge)>0){
        t = std::max(t,0.);
        t = std::min(t,1.);
        double start, end;
        Handle_Geom_Curve c = BRep_Tool::Curve(edge, start, end);
        if (edge.Orientation()==TopAbs_REVERSED){
            t = 1.-t;
        }
        return c->Value(t*(end-start)+start);
    } else {
        TopExp_Explorer ex(edge,TopAbs_VERTEX);
        if (ex.More()){
            gp_Pnt pnt(BRep_Tool::Pnt(TopoDS::Vertex(ex.Current())));
            return pnt;
        } else {
            return gp_Pnt(0.,0.,0.);

        }
    }
    //  }
    //  throw( PGError("edgeAt: Given edge is undefined."));
    //  return gp_Pnt(0,0,0);
}


namespace {

arma::mat tangentAt(const Adaptor3d_Curve &curve, const gp_Pnt &P)
{
    const Standard_Real uf = curve.FirstParameter();
    const Standard_Real ul = curve.LastParameter();

    // Extrema_ExtPC only reports interior (perpendicular) extrema, so seed
    // the search with the closer of the two endpoints as a robust fallback.
    Standard_Real bestU  = uf;
    Standard_Real bestD2 = P.SquareDistance(curve.Value(uf));
    const Standard_Real d2l = P.SquareDistance(curve.Value(ul));
    if (d2l < bestD2) { bestD2 = d2l; bestU = ul; }

    Extrema_ExtPC proj(P, curve, Precision::Confusion());
    if (proj.IsDone()) {
        for (Standard_Integer i = 1; i <= proj.NbExt(); ++i) {
            const Standard_Real d2 = proj.SquareDistance(i);
            if (d2 < bestD2) {
                bestD2 = d2;
                bestU  = proj.Point(i).Parameter();
            }
        }
    }

    gp_Pnt onCurve;
    gp_Vec d1;
    curve.D1(bestU, onCurve, d1);

    return normalized(vec3(d1));
}

}


arma::mat edgeTangent(TopoDS_Shape edge, const arma::mat &pt)
{
    arma::mat T;

    auto eval = [&](const Adaptor3d_Curve &curve, bool reverse)
    {
        auto Pi=cad::to_Pnt(pt);
        T = tangentAt(curve, Pi);
        if (reverse) T = -T;
    };

    switch (edge.ShapeType()) {
    case TopAbs_EDGE: {
        const TopoDS_Edge e = TopoDS::Edge(edge);
        BRepAdaptor_Curve curve(e);
        // Optional: align the tangent with the edge's topological direction.
        eval(curve, e.Orientation() == TopAbs_REVERSED);
        break;
    }
    case TopAbs_WIRE: {
        BRepAdaptor_CompCurve curve(TopoDS::Wire(edge));
        eval(curve, edge.Orientation() == TopAbs_REVERSED);
        break;
    }
    default:
        throw std::invalid_argument(
            "edgeTangent: shape must be a TopoDS_Edge or TopoDS_Wire.");
    }

    return T;

}


}
}
