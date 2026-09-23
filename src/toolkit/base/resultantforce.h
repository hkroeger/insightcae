/*
 * This file is part of Insight CAE, a workbench for Computer-Aided Engineering
 * Copyright (C) 2014  Hannes Kroeger <hannes@kroegeronline.net>
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


#ifndef INSIGHT_RESULTANTFORCE_H
#define INSIGHT_RESULTANTFORCE_H

#include "base/linearalgebra.h"

namespace insight {

struct ResultantForce
{
    /**
     * @brief F
     * the resultant force
     */
    arma::mat F;

    /**
     * @brief r
     * the vector from original center c to the
     * resultant force attack point
     */
    arma::mat r;

    /**
     * @brief Mr
     * the residual moment around the resultant force attack point
     */
    arma::mat Mr;

    /**
     * @brief ResultantForce
     * @param F_c
     * the sum of forces
     * @param M_c
     * the moments around some center c
     */
    ResultantForce(
        const arma::mat& F_c,
        const arma::mat& M_c );
};

}

#endif // INSIGHT_RESULTANTFORCE_H
