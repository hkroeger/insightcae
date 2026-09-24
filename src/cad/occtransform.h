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

#ifndef INSIGHT_OCCTRANSFORM_H
#define INSIGHT_OCCTRANSFORM_H

#include "base/spatialtransformation.h"
#include "occinclude.h"

namespace insight {
namespace cad {

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

}
}

#endif // INSIGHT_OCCTRANSFORM_H
