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

#ifndef INSIGHT_CAD_ISPARTOFSOLID_H
#define INSIGHT_CAD_ISPARTOFSOLID_H

#include "feature.h"
#include "base/exception.h"

namespace insight {
namespace cad {


template<EntityType T>
class isPartOfSolid
    : public Filter
{
protected:
    TopoDS_Solid s_;

public:
    isPartOfSolid(const TopoDS_Solid& s)
    : s_(s)
    {
    }

    isPartOfSolid(FeaturePtr m)
    : s_(*m)
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    isPartOfSolid(FeatureSet f)
        : s_(TopoDS::Solid(static_cast<TopoDS_Shape>(*f.model())))
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new isPartOfSolid(s_));
    }

};


template<> isPartOfSolid<Edge>::isPartOfSolid(FeaturePtr m);
template<> bool isPartOfSolid<Edge>::checkMatch(FeatureID feature) const;
template<> isPartOfSolid<Face>::isPartOfSolid(FeaturePtr m);
template<> bool isPartOfSolid<Face>::checkMatch(FeatureID feature) const;

typedef isPartOfSolid<Edge> isPartOfSolidEdge;
typedef isPartOfSolid<Face> isPartOfSolidFace;


}
}

#endif // INSIGHT_CAD_ISPARTOFSOLID_H
