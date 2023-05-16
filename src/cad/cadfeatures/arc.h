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

#ifndef INSIGHT_CAD_ARC_H
#define INSIGHT_CAD_ARC_H

#include "cadparameters.h"
#include "cadfeatures/singleedgefeature.h"

namespace insight 
{
namespace cad
{

    
    
    
class Arc
    : public SingleEdgeFeature
{
    VectorPtr p0_;
    VectorPtr p0tang_;
    VectorPtr p1_;

    size_t calcHash() const override;
    void build() override;

    Arc ( VectorPtr p0, VectorPtr p0tang, VectorPtr p1 );

public:
    declareType ( "Arc" );

    CREATE_FUNCTION(Arc);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    VectorPtr start() const override { return p0_; }
    VectorPtr end() const  override { return p1_; }

    bool isSingleOpenWire() const override;
};

    
    
    
class Arc3P
    : public SingleEdgeFeature
{
    VectorPtr p0_;
    VectorPtr pm_;
    VectorPtr p1_;

    size_t calcHash() const override;
    void build() override;

    Arc3P ( VectorPtr p0, VectorPtr pm, VectorPtr p1 );

public:
    declareType ( "Arc3P" );
    
    CREATE_FUNCTION(Arc3P);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    VectorPtr start() const override { return p0_; }
    VectorPtr end() const  override { return p1_; }

    bool isSingleOpenWire() const override;

};



}
}

#endif // INSIGHT_CAD_ARC_H
