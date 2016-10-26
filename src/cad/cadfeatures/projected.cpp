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

#include "projected.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Projected);
addToFactoryTable(Feature, Projected, NoParameters);




Projected::Projected(const NoParameters& nop): Feature(nop)
{}




Projected::Projected(FeaturePtr source, FeaturePtr target, VectorPtr dir)
: source_(source), target_(target), dir_(dir)
{
}




FeaturePtr Projected::create ( FeaturePtr source, FeaturePtr target, VectorPtr dir )
{
    return FeaturePtr(new Projected(source, target, dir));
}





void Projected::build()
{
  TopoDS_Wire ow=BRepTools::OuterWire(TopoDS::Face(source_->shape()));

  BRepProj_Projection proj(ow, target_->shape(), to_Vec(dir_->value()));
  
  setShape(proj.Shape());
}




void Projected::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Projected",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::bind(&Projected::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Projected::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Projected",
            "( <feature:base>, <feature:target>, <vector:dir> )",
            "Projects the feature base onto the feature target. The direction has to be prescribed by vector dir."
        )
    );
}



}
}
