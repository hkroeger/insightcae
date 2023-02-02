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

#include "cadfeature.h"
#include "feature.h"
#include "base/exception.h"

namespace insight {

namespace cad {


template<EntityType T>
class isPartOfSolid
    : public Filter
{
protected:
    std::vector<TopoDS_Solid> s_;

public:
    isPartOfSolid(const std::vector<TopoDS_Solid>& s)
        : s_(s)
    {
    }

    isPartOfSolid(FeaturePtr m)
      : s_({TopoDS::Solid(m->shape())})
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    isPartOfSolid(FeatureSet f)
    {
        insight::assertion(f.shape()==Solid, "expected set of solids");
        for (auto& i: f.data())
        {
            TopoDS_Solid s=f.model()->subsolid(i);
            BRepTools::Dump(s, std::cout);
            s_.push_back(s);
        }
    }

    bool checkMatch(FeatureID) const
    {
        throw insight::Exception("isPartOfSolid filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new isPartOfSolid(s_));
    }

};


template<> bool isPartOfSolid<Edge>::checkMatch(FeatureID feature) const;
template<> bool isPartOfSolid<Face>::checkMatch(FeatureID feature) const;

typedef isPartOfSolid<Edge> isPartOfSolidEdge;
typedef isPartOfSolid<Face> isPartOfSolidFace;


}
}

#endif // INSIGHT_CAD_ISPARTOFSOLID_H
