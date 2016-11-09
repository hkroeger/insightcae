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

#include "linearpattern.h"
#include "transform.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

    
    
    
defineType(LinearPattern);
addToFactoryTable(Feature, LinearPattern);




LinearPattern::LinearPattern(): Compound()
{}



  
LinearPattern::LinearPattern(FeaturePtr m1, VectorPtr axis, ScalarPtr n)
: m1_(m1), axis_(axis), n_(n)
{}




FeaturePtr LinearPattern::create ( FeaturePtr m1, VectorPtr axis, ScalarPtr n )
{
    return FeaturePtr(new LinearPattern(m1, axis, n));
}




void LinearPattern::build()
{
//   BRep_Builder bb;
//   TopoDS_Compound result;
//   bb.MakeCompound(result);

    double delta_x=norm ( axis_->value(), 2 );
    gp_Vec ax ( to_Vec ( axis_->value() /delta_x ) );

    int n=round ( n_->value() );

    int j=0;
    CompoundFeatureMap instances;

    for ( int i=0; i<n; i++ ) {
        gp_Trsf tr;
        tr.SetTranslation ( ax*delta_x*double ( i ) );
//     bb.Add(result, BRepBuilderAPI_Transform(m1_->shape(), tr).Shape());

        components_[str ( format ( "component%d" ) % ( j+1 ) )] = FeaturePtr ( new Transform ( m1_, tr ) );
        j++;
    }

    m1_->unsetLeaf();
    Compound::build();

//   setShape(result);
}




void LinearPattern::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "LinearPattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > 
      ',' > ruleset.r_vectorExpression > 
      ',' > ruleset.r_scalarExpression > ')' ) 
      [ qi::_val = phx::bind(&LinearPattern::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList LinearPattern::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "LinearPattern",
         
            "( <feature:base>, <vector:delta_l>, <scalar:n> )",
         
            "Copies the bease feature base into a linear pattern."
            " The copies of the base feature are shifted in increments of delta_l."
            " The number of copies is n."
        )
    );
}



}
}
