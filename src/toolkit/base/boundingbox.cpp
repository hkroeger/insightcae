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


#include "boundingbox.h"

#include <cfloat>
#include <iostream>

namespace insight
{

arma::mat initializedBndBox()
{
    arma::mat bb=arma::zeros(3,2);
    bb.col(0)=arma::ones(3,1)*DBL_MAX;
    bb.col(1)=arma::ones(3,1)*(-DBL_MAX);
    return bb;
}

arma::mat unitedBndBox(const arma::mat& bb1, const arma::mat& bb2)
{
    arma::mat bbm = ArmaMatCmpts{
        { std::min(bb1(0,0),bb2(0,0)), std::max(bb1(0,1),bb2(0,1)) },
        { std::min(bb1(1,0),bb2(1,0)), std::max(bb1(1,1),bb2(1,1)) },
        { std::min(bb1(2,0),bb2(2,0)), std::max(bb1(2,1),bb2(2,1)) }
    };
    return bbm;
}

arma::mat computeOffsetContour(const arma::mat &pl, double thickness, const arma::mat &normals)
{
    arma::mat pl2 = arma::zeros(pl.n_rows, pl.n_cols); //not existing in older armadillo: arma::reverse(pl, 0);
    for (arma::uword i=0; i<pl.n_rows; ++i)
        pl2.row(i)=pl.row(pl.n_rows-1-i);

    arma::mat lp;
    for (arma::uword i=0; i<pl2.n_rows; ++i)
    {
        auto p=pl2.row(i);
        auto n=normals.row(i);

        arma::mat t;
        if (i>0)
        {
            t = p-lp;
        }
        else
        {
            t = pl2.row(i+1)-p;
        }
        lp = p;
        t/=arma::norm(t,2);

        arma::mat th = arma::cross(n, t);
        std::cout<<p<<n<<t<<th<<std::endl;
        th /= arma::norm(th,2);
        pl2.row(i) += th * thickness;
    }
    return arma::join_vert(pl, pl2);
}

}
