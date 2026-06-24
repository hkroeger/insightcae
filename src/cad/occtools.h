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
