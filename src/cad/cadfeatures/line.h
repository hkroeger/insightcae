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

#ifndef INSIGHT_CAD_LINE_H
#define INSIGHT_CAD_LINE_H

#include "singleedgefeature.h"
#include "constrainedsketchgeometry.h"

namespace insight {
namespace cad {
    
    


class Line
    : public SingleEdgeFeature,
      public ConstrainedSketchEntity
{
    VectorPtr p0_, p1_;
    bool second_is_dir_;

    Line ( VectorPtr p0, VectorPtr p1, bool second_is_dir=false );


    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "Line" );
    Line ();

    static FeaturePtr create ( VectorPtr p0, VectorPtr p1 );
    static FeaturePtr create_dir ( VectorPtr p0, VectorPtr dir );

    void insertrule ( parser::ISCADParser& ruleset ) const override;
    FeatureCmdInfoList ruleDocumentation() const override;

    VectorPtr start() const override;
    VectorPtr end() const override;

    void scaleSketch(double scaleFactor) override;
};




}
}

#endif // INSIGHT_CAD_LINE_H
