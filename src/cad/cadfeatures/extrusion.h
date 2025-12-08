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

#ifndef INSIGHT_CAD_EXTRUSION_H
#define INSIGHT_CAD_EXTRUSION_H

#include "cadtypes.h"
#include "cadparameters.h"
#include "cadfeature.h"

namespace insight {
namespace cad {

    
    
    
class Extrusion
    : public Feature
{
    FeaturePtr sk_;
    boost::variant<VectorPtr,ScalarPtr> L_;
    bool centered_;

    Extrusion(const Extrusion&o, TreeCloneMap& tcm);
    Extrusion ( FeaturePtr sk, ScalarPtr L, bool centered=false );
    Extrusion ( FeaturePtr sk, VectorPtr L, bool centered=false );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "Extrusion" );
#ifndef SWIG
    DEPENDS((sk_, L_));
#endif
    CREATE_FUNCTION(Extrusion);
    CLONEABLE(Extrusion);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};




}
}

#endif // INSIGHT_CAD_EXTRUSION_H
