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

#include "occtools.h"
#include "cadexception.h"
#include "cadfeature.h"
#include "datum.h"
#include "base/linearalgebra.h"
#include "base/spatialtransformation.h"
#include "base/units.h"

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


is_gp_Trsf::is_gp_Trsf()
{}


is_gp_Trsf::is_gp_Trsf(const gp_Trsf &trsf)
{
    gp_Trsf::operator=(trsf);
}

is_gp_Trsf::is_gp_Trsf(const insight::SpatialTransformation& trsf)
    : is_gp_Trsf(trsf.translate(), trsf.rollPitchYaw(), trsf.scale())
{}

is_gp_Trsf::is_gp_Trsf(const arma::mat& translate, const arma::mat& rollPitchYaw, double scale)
{
    gp_Trsf::operator=(OFtransformToOCC(translate, rollPitchYaw, scale));
}

is_gp_Trsf::is_gp_Trsf(
    const arma::mat& translate,
    const arma::mat& rollPitchYaw,
    const arma::mat& scale )
{
    double s=scale[0];
    insight::assertion(
      (fabs(scale[1]-scale[0])<SMALL)
        && (fabs(scale[2]-scale[0])<SMALL),
        "unequal scaling factors for different directions are not supported" );

    gp_Trsf::operator=(OFtransformToOCC(translate, rollPitchYaw, s));
}


insight::SpatialTransformation is_gp_Trsf::toSpatialTransformation() const
{
    double s = ScaleFactor();

    arma::mat R = arma::zeros(3,3);
    for (int i=0;i<3;i++)
        for (int j=0;j<3;j++)
            R(i,j)=Value(i+1,j+1);

    R*=1./s;

    insight::SpatialTransformation tt;
    tt.setScale(s);
    tt.setRotationMatrix(R);
    tt.setTranslation(
        (1./s)*inv(R)*insight::Vector(TranslationPart())
    );
    return tt;
}

is_gp_Trsf::operator insight::SpatialTransformation() const
{
    return toSpatialTransformation();
}


gp_Trsf is_gp_Trsf::OFtransformToOCC(
    const arma::mat &translate,
    const arma::mat &rollPitchYaw,
    double scale )
{
    gp_Trsf tr; tr.SetTranslation(to_Vec(translate));
    gp_Trsf rz; rz.SetRotation(gp::OZ(), rollPitchYaw(2)*SI::deg);
    gp_Trsf ry; ry.SetRotation(gp::OY(), rollPitchYaw(1)*SI::deg);
    gp_Trsf rx; rx.SetRotation(gp::OX(), rollPitchYaw(0)*SI::deg);
    gp_Trsf sc; sc.SetScaleFactor(scale);
    return sc*rx*ry*rz*tr;
}




// OCCtransformToOF::OCCtransformToOF(const gp_Trsf &t)
// {
//   scale_ = t.ScaleFactor();

//   arma::mat R = arma::zeros(3,3);
//   for (int i=0;i<3;i++)
//     for (int j=0;j<3;j++)
//       R(i,j)=t.Value(i+1,j+1);

//   R*=1./scale_;

//   R_ = R;

//   translate_ = (1./scale_)*inv(R)*insight::Vector(t.TranslationPart());
// }




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
