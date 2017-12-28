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

#include "line.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


    
    
defineType(Line);
addToFactoryTable(Feature, Line);




Line::Line()
: Feature()
{
}




Line::Line(VectorPtr p0, VectorPtr p1)
: p0_(p0), p1_(p1)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=p0_->value();
  h+=p1_->value();
}



FeaturePtr Line::create ( VectorPtr p0, VectorPtr p1 )
{
    return FeaturePtr(new Line(p0, p1));
}




void Line::build()
{
  refpoints_["p0"]=p0_->value();  
  refpoints_["p1"]=p1_->value();
//   refvalues_["L"]=arma::norm(p1-p0, 2);
  refvectors_["ex"]=(p1_->value() - p0_->value())/arma::norm(p1_->value() - p0_->value(), 2);
  
  setShape(BRepBuilderAPI_MakeEdge(
      GC_MakeSegment(to_Pnt(p0_->value()), to_Pnt(p1_->value())).Value()
  ));
}




void Line::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Line",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::bind(&Line::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Line::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Line",
         
            "( <vector:p0>, <vector:p1> )",
         
            "Creates a line between point p0 and p1."
        )
    );
}



bool Line::isSingleEdge() const
{
    return true;
}


bool Line::isSingleCloseWire() const
{
  return false;
}




bool Line::isSingleOpenWire() const
{
  return true;
}




}
}
