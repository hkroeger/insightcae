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

#include "fixshape.h"
#include "base/boost_include.h"
#include "base/translations.h"

#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(FixShape);
//addToFactoryTable(Feature, FixShape);
addToStaticFunctionTable(Feature, FixShape, insertrule);
addToStaticFunctionTable(Feature, FixShape, ruleDocumentation);



size_t FixShape::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+= *in_;
  return h.getHash();
}





FixShape::FixShape ( FeaturePtr in )
    : in_ ( in )
{
}







void FixShape::build()
{
    ShapeFix_Shape FixShape;
    FixShape.Init ( in_->shape() );
    FixShape.Perform();

    setShape ( FixShape.Shape() );
}




FixShape::operator const TopoDS_Shape& () const
{
    return shape();
}




void FixShape::insertrule ( parser::ISCADParser& ruleset )
{
    ruleset.modelstepFunctionRules.add
    (
        "FixShape",
        typename parser::ISCADParser::ModelstepRulePtr ( new typename parser::ISCADParser::ModelstepRule (

                    ( '(' >> ruleset.r_solidmodel_expression >> ')' )
                    [ qi::_val = phx::bind(
                         &FixShape::create<FeaturePtr>,
                         qi::_1) ]

                ) )
    );
}




FeatureCmdInfoList FixShape::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "FixShape",

            "( <feature:in> )",

            _("Runs some repair operations on output shape of feature in.")
        )
    };
}



}
}
