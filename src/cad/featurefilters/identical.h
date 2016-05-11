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

#ifndef INSIGHT_CAD_IDENTICAL_H
#define INSIGHT_CAD_IDENTICAL_H


#include "feature.h"
#include "base/exception.h"

namespace insight 
{
namespace cad 
{

template<EntityType T>
class identical
: public Filter
{
protected:
    FeatureSet f_;

public:
    identical(FeaturePtr m)
    : f_(m, T)
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    identical(FeatureSet f)
    : f_(f)
    {}

    bool checkMatch(FeatureID feature) const
    {
        throw insight::Exception("coincident filter: not implemented!");
    }

    FilterPtr clone() const
    {
        return FilterPtr(new identical(f_));
    }

};


template<> identical<Edge>::identical(FeaturePtr m);
template<> bool identical<Edge>::checkMatch(FeatureID feature) const;
template<> identical<Face>::identical(FeaturePtr m);
template<> bool identical<Face>::checkMatch(FeatureID feature) const;
// template<> identical<Solid>::coincident(FeaturePtr m);
// template<> bool identical<Solid>::checkMatch(FeatureID feature) const;

typedef identical<Edge> identicalEdge;
typedef identical<Face> identicalFace;
// typedef identical<Solid> coincidentSolid;


}
}


#endif // INSIGHT_CAD_IDENTICAL_H
