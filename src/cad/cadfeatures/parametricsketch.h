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

#ifndef INSIGHT_CAD_PARAMETRICSKETCH_H
#define INSIGHT_CAD_PARAMETRICSKETCH_H

#include "derivedfeature.h"

namespace insight {
namespace cad {

class ConstrainedSketch;



/**
 * @brief The ParametricSketch class
 * wraps a ConstrainedSketch, which is defined inline in the ISCAD script.
 * The location of the sketch script inside the ISCAD script is remembered,
 * so that the GUI can replace it by the result of the ConstrainedSketchEditor.
 */
class ParametricSketch
    : public DerivedFeature
{
    std::size_t scriptBegin_, scriptEnd_;
    bool hasScriptArgument_;

    ParametricSketch(const ParametricSketch&o, TreeCloneMap& tcm);
    ParametricSketch (
        std::shared_ptr<ConstrainedSketch> sketch,
        std::size_t scriptBegin,
        std::size_t scriptEnd,
        bool hasScriptArgument );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "ParametricSketch" );

    CREATE_FUNCTION(ParametricSketch);
    CLONEABLE(ParametricSketch);

    std::shared_ptr<const ConstrainedSketch> sketch() const;

    /**
     * @brief scriptRange
     * @return character range of the sketch script argument in the ISCAD script.
     * If there is no sketch script argument, begin==end is the position of the closing bracket.
     */
    std::pair<std::size_t,std::size_t> scriptRange() const;

    /**
     * @brief hasScriptArgument
     * @return false, if the statement was given without the sketch script argument
     * (i.e. without the comma after the plane)
     */
    bool hasScriptArgument() const;

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();
};




}
}

#endif // INSIGHT_CAD_PARAMETRICSKETCH_H
