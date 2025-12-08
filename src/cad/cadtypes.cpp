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

#include "cadtypes.h"
#include "cadfeature.h"
#include "datum.h"

#include "cadfeatures/compound.h"
#include "cadfeatures/importsolidmodel.h"

namespace insight 
{
namespace cad 
{



VisualizationStyle::VisualizationStyle()
{}

VisualizationStyle::VisualizationStyle(
    boost::variant<boost::blank,DatasetRepresentation> s,
    const arma::mat& c,
    boost::variant<boost::blank,double> o )
  : style(s), color(c), opacity(o)
{}


FeatureVisualizationStyle::FeatureVisualizationStyle()
{}

FeatureVisualizationStyle::FeatureVisualizationStyle(
        boost::variant<boost::blank,DatasetRepresentation> s,
        const arma::mat& c,
        const std::vector<std::string> app,
        boost::variant<boost::blank,double> o,
        bool initiallyVisible )
  : VisualizationStyle(s, c, o),
    associatedParameterPaths(app),
    initiallyVisible(initiallyVisible)
{}


FeatureVisualizationStyle FeatureVisualizationStyle::componentStyle()
{
    static std::unique_ptr<FeatureVisualizationStyle> cs;
    if (!cs)
    {
        cs.reset(new FeatureVisualizationStyle());
        cs->style=insight::DatasetRepresentation::Surface;
        cs->opacity=1.;
        cs->initiallyVisible=true;
    }
    return *cs;
}

FeatureVisualizationStyle FeatureVisualizationStyle::intermediateFeatureStyle()
{
    static std::unique_ptr<FeatureVisualizationStyle> ifs;
    if (!ifs)
    {
        ifs.reset(new FeatureVisualizationStyle());
        ifs->style=insight::DatasetRepresentation::Surface;
        ifs->opacity=0.5;
        ifs->initiallyVisible=false;
    }
    return *ifs;
}





ModelVariableTable mergeMVTs(const ModelVariableTable& mvt1, const ModelVariableTable& mvt2)
{
    ModelVariableTable rmvt(mvt1.begin(), mvt1.end());

    std::copy(mvt2.begin(), mvt2.end(),
              std::back_inserter(rmvt) );

    return rmvt;
}




}
}
