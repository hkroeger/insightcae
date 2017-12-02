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

#include "circle.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Circle);
addToFactoryTable(Feature, Circle);




Circle::Circle()
{
}




void Circle::build()
{
  Handle_Geom_Curve c=GC_MakeCircle(to_Pnt(p0_->value()), to_Vec(n_->value()), 0.5*D_->value());
  
  refpoints_["p0"]=p0_->value();
  
  BRepBuilderAPI_MakeEdge e(c);
  BRepBuilderAPI_MakeWire w;
  w.Add(e.Edge());
  providedSubshapes_["OuterWire"].reset(new Feature(e.Edge()));
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}




Circle::Circle(VectorPtr p0, VectorPtr n, ScalarPtr D)
: p0_(p0), n_(n), D_(D)
{
    ParameterListHash h(this);
    h+=this->type();
    h+=p0_->value();
    h+=n_->value();
    h+=D_->value();
}




FeaturePtr Circle::create(VectorPtr p0, VectorPtr n, ScalarPtr D)
{
    return FeaturePtr(new Circle(p0, n, D));
}



Circle::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape());
}




void Circle::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Circle",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_scalarExpression >> ')' ) 
	[ qi::_val = phx::bind(&Circle::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Circle::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Circle",
         
            "( <vector:p0>, <vector:n>, <scalar:D> )",
            
            "Creates a circular face. The circle is centered at p0, the axis vector is n and the diameter D."
        )
    );
}



}
}
