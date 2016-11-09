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

#include "sphere.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

    
    
    
defineType(Sphere);
addToFactoryTable(Feature, Sphere);




Sphere::Sphere()
{}



  
Sphere::Sphere(VectorPtr p, ScalarPtr D)
: p_(p), D_(D)
{}



FeaturePtr Sphere::create ( VectorPtr p, ScalarPtr D )
{
    return FeaturePtr(new Sphere(p, D));
}




void Sphere::build()
{
  setShape
  (
    BRepPrimAPI_MakeSphere
    (
      to_Pnt(p_->value()),
      0.5*D_->value()
    ).Shape()
  );
}




void Sphere::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Sphere",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	[ qi::_val = phx::bind(&Sphere::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Sphere::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Sphere",
            "( <vector:p0>, <scalar:D> )",
            "Creates a sphere around point p0 with diameter D."
        )
    );
}



}
}
