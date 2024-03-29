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

#ifndef SINEWAVE_H
#define SINEWAVE_H

#include "cadfeature.h"
#include "cadfeatures/singleedgefeature.h"

namespace insight {
namespace cad {




class SineWave
    : public SingleEdgeFeature
{
    ScalarPtr l_, A_;

    SineWave ( ScalarPtr l, ScalarPtr A );

    size_t calcHash() const override;
    void build() override;

public:
    declareType ( "SineWave" );

    CREATE_FUNCTION(SineWave);

    static void insertrule ( parser::ISCADParser& ruleset );
    static FeatureCmdInfoList ruleDocumentation();

    bool isSingleEdge() const override
    {
        return true;
    };

};

}
}

#endif // SINEWAVE_H
