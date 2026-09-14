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

#ifndef INSIGHT_OCCTOOLS_H
#define INSIGHT_OCCTOOLS_H

#include "base/spatialtransformation.h"
#include "occinclude.h"

namespace insight {
namespace cad {

bool isShapeEmpty(const TopoDS_Shape& shape);

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


arma::mat calcBndBox(TopoDS_Shape);

class is_gp_Trsf
    : public gp_Trsf
{
    static
        gp_Trsf OFtransformToOCC(
            const arma::mat& translate,
            const arma::mat& rollPitchYaw,
            double scale );
public:
    is_gp_Trsf();
    is_gp_Trsf(const gp_Trsf& trsf);
    is_gp_Trsf(const insight::SpatialTransformation& trsf);
    is_gp_Trsf(const arma::mat& translate, const arma::mat& rollPitchYaw, double scale);
    is_gp_Trsf(const arma::mat& translate, const arma::mat& rollPitchYaw, const arma::mat& scale);

    insight::SpatialTransformation toSpatialTransformation() const;
    operator insight::SpatialTransformation() const;

};


// gp_Trsf OFtransformToOCC(const insight::SpatialTransformation& trsf);

// class OCCtransformToOF
//         : public insight::SpatialTransformation
// {
// public:
//   OCCtransformToOF(const gp_Trsf& t);
// };


std::vector<arma::mat> orderedCornerPoints(const TopoDS_Shape& f);
TopoDS_Face asSingleFace(const TopoDS_Shape& shape);

double edgeLength(const TopoDS_Edge& e);

std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, double approxSegmentLength, double* actualSegmentLength=NULL, int minSegments=1);
std::vector<gp_Pnt> resampleEdgeUniform(const TopoDS_Edge& edge, int nSegments);
std::vector<double> resampleEdge (const TopoDS_Edge& edge, const std::vector<double> lIn, int c0Res=-1);

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

gp_Pnt edgeAt(const TopoDS_Edge& edge, double t);
gp_Pnt faceAt(const TopoDS_Face& face, gp_Pnt2d p2d);
std::vector<gp_Pnt> faceAt
    (
        const TopoDS_Face& face,
        std::vector<gp_XY>::const_iterator begin,
        std::vector<gp_XY>::const_iterator end
        );
gp_Pnt faceAt(const TopoDS_Face& face, double u, double v );

Bnd_Box getBoundingBox(const TopoDS_Shape& shape, double deflection=-1);
Bnd_Box getBoundingBox(const TopoDS_Shape& shape, gp_Pnt& bbMin, gp_Pnt& bbMax, double deflection=-1 );


arma::mat edgeTangent(TopoDS_Shape edge, const arma::mat& pt);

template<class Trsf>
void
transformTriangulation(
    TopoDS_Shape original,
    TopoDS_Shape& transformed,
    const Trsf& tr
    )
{
    // transform triangulation as well
    BRep_Builder aB;

    TopTools_IndexedMapOfShape orgFaces, trsfFaces;
    TopExp::MapShapes(original, TopAbs_FACE, orgFaces);
    TopExp::MapShapes(transformed, TopAbs_FACE, trsfFaces);

    for (int i=1; i<=orgFaces.Extent(); ++i)
    {
        TopLoc_Location lt;
        TopoDS_Face ft=TopoDS::Face(trsfFaces.FindKey(i));
        if (!BRep_Tool::Triangulation(ft, lt))
        {
            TopLoc_Location lo;
            TopoDS_Face fo=TopoDS::Face(orgFaces.FindKey(i));
            if (auto otri=BRep_Tool::Triangulation(fo, lo))
            {
                gp_Trsf lotr=lo;
                gp_Trsf ltr=lt; ltr.Invert();
                auto tri=otri->Copy();

                for (int i=1; i<=tri->NbNodes(); ++i)
                {
                    auto xyz=tri->Nodes().Value(i).XYZ();
                    lotr.Transforms(xyz);
                    tr.Transforms(xyz);
                    ltr.Transforms(xyz);
                    tri->ChangeNodes().ChangeValue(i)=xyz;
                }

                aB.UpdateFace(ft, tri);
            }
        }
    }
}




}
}

#endif // INSIGHT_OCCTOOLS_H
