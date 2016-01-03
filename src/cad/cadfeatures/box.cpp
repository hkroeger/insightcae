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

#include "box.h"

#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {


defineType(Box);
addToFactoryTable(Feature, Box, NoParameters);

Box::Box(const NoParameters&)
{}

  
Box::Box
(
  VectorPtr p0, 
  VectorPtr L1, 
  VectorPtr L2, 
  VectorPtr L3,
  bool centered
)
: p0_(p0), L1_(L1), L2_(L2), L3_(L3), centered_(centered)
{}

void Box::build()
{ 
  refpoints_["p0"]=p0_->value();
  
  refvalues_["L1"]=arma::norm(L1_->value(), 2);
  refvalues_["L2"]=arma::norm(L2_->value(), 2);
  refvalues_["L3"]=arma::norm(L3_->value(), 2);
  
  refvectors_["e1"]=L1_->value()/arma::norm(L1_->value(), 2);
  refvectors_["e2"]=L2_->value()/arma::norm(L2_->value(), 2);
  refvectors_["e3"]=L3_->value()/arma::norm(L3_->value(), 2);
  
  Handle_Geom_Plane pln=GC_MakePlane(to_Pnt(p0_->value()), to_Pnt(p0_->value()+L1_->value()), to_Pnt(p0_->value()+L2_->value())).Value();
  TopoDS_Shape box=
  BRepPrimAPI_MakePrism
  (
    BRepBuilderAPI_MakeFace
    (
      pln,
      BRepBuilderAPI_MakePolygon
      (
	to_Pnt(p0_->value()), 
	to_Pnt(p0_->value()+L1_->value()), 
	to_Pnt(p0_->value()+L1_->value()+L2_->value()), 
	to_Pnt(p0_->value()+L2_->value()), 
	true
      ).Wire()
    ).Face(),
    to_Vec(L3_->value())
  ).Shape();
  
  if (centered_)
  {
    gp_Trsf t;
    t.SetTranslation(to_Vec(-0.5*L1_->value() - 0.5*L2_->value() - 0.5*L3_->value()));
    box=BRepBuilderAPI_Transform(box, t).Shape();
  }
  setShape(box);
}

void Box::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Box",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
		    > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
     > ( (  ',' > qi::lit("centered") > qi::attr(true) ) |qi::attr(false) )
     > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Box>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

}
}
