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

#include "occtransform.h"
#include "occconvert.h"
#include "base/units.h"
#include "base/exception.h"

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

}
}
