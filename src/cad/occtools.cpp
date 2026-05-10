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
