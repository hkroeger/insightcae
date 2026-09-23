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


#include "resultantforce.h"

#include <iostream>

namespace insight
{

ResultantForce::ResultantForce(
    const arma::mat &F_c,
    const arma::mat &M_c )
    : F(F_c)
{
    arma::mat MR{
        {  0.,       F_c(2),   -F_c(1)  },
        { -F_c(2),   0.,        F_c(0)  },
        {  F_c(1),  -F_c(0),    0.      }
    };
    r = arma::pinv(MR) * M_c;
    Mr = M_c - arma::cross(r, F_c);

    std::cout
        <<"F_c:\n"<<F_c
        <<"M_c:\n"<<M_c
        <<"MR:\n"<<MR
        <<"r:\n"<<r
        <<"Mr:\n"<<Mr;
}

}
