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
 */

#ifndef INSIGHT_CAD_COINCIDENT_H
#define INSIGHT_CAD_COINCIDENT_H

#include "feature.h"
#include "base/exception.h"
#include "constantquantity.h"

namespace insight 
{
namespace cad 
{

template<EntityType T>
class coincident
: public Filter
{
protected:
    FeatureSet f_;
    scalarQuantityComputerPtr tol_;

public:
    coincident(FeaturePtr m, scalarQuantityComputerPtr tol = scalarQuantityComputerPtr(new constantQuantity<double>(1e-3) ) )
    : f_(m, T), tol_(tol)
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    coincident(FeatureSet f, scalarQuantityComputerPtr tol = scalarQuantityComputerPtr(new constantQuantity<double>(1e-3) ) )
    : f_(f), tol_(tol)
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new coincident(f_, tol_));
    }

};


template<> coincident<Edge>::coincident(FeaturePtr m, scalarQuantityComputerPtr tol);
template<> bool coincident<Edge>::checkMatch(FeatureID feature) const;
template<> coincident<Face>::coincident(FeaturePtr m, scalarQuantityComputerPtr tol);
template<> bool coincident<Face>::checkMatch(FeatureID feature) const;
template<> coincident<Solid>::coincident(FeaturePtr m, scalarQuantityComputerPtr tol);
template<> bool coincident<Solid>::checkMatch(FeatureID feature) const;

typedef coincident<Edge> coincidentEdge;
typedef coincident<Face> coincidentFace;
typedef coincident<Solid> coincidentSolid;


}
}

#endif // INSIGHT_CAD_COINCIDENT_H
