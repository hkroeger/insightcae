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

#include "cylinder.h"
#include "base/boost_include.h"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(Cylinder);
addToFactoryTable(Feature, Cylinder, NoParameters);

Cylinder::Cylinder(const NoParameters&)
{}


Cylinder::Cylinder(VectorPtr p1, VectorPtr p2, ScalarPtr D, bool p2isAxis)
: p2isAxis_(p2isAxis), p1_(p1), p2_(p2), D_(D)
{}

Cylinder::Cylinder(VectorPtr p1, VectorPtr p2, ScalarPtr Da, ScalarPtr Di, bool p2isAxis)
: p2isAxis_(p2isAxis), p1_(p1), p2_(p2), D_(Da), Di_(Di)
{}

void Cylinder::build()
{
  refpoints_["p0"]=p1_->value();
  
  arma::mat p2;
  if (p2isAxis_)
  {
    p2=p1_->value()+p2_->value();
  }
  else
  {
    p2=p2_->value();
  }
  refpoints_["p1"]=p2;
  refvalues_["Da"]=D_->value();
  
  TopoDS_Shape cyl=
    BRepPrimAPI_MakeCylinder
    (
      gp_Ax2
      (
	gp_Pnt(p1_->value()(0),p1_->value()(1),p1_->value()(2)), 
	gp_Dir(p2(0) - p1_->value()(0), p2(1) - p1_->value()(1), p2(2) - p1_->value()(2))
      ),
      0.5*D_->value(), 
      norm(p2 - p1_->value(), 2)
    ).Shape();
    
  if (Di_)
  {
    refvalues_["Di"]=Di_->value();
    cyl=BRepAlgoAPI_Cut
    (
      
      cyl,
     
      BRepPrimAPI_MakeCylinder
      (
	gp_Ax2
	(
	  gp_Pnt(p1_->value()(0),p1_->value()(1),p1_->value()(2)), 
	  gp_Dir(p2(0) - p1_->value()(0), p2(1) - p1_->value()(1), p2(2) - p1_->value()(2))
	),
	0.5*Di_->value(), 
	norm(p2 - p1_->value(), 2)
      ).Shape()
      
    );
  }

  setShape(cyl);
}

void Cylinder::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cylinder",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ( ( qi::lit("ax") >> qi::attr(true) ) | qi::attr(false) )
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_scalarExpression 
      >> ( (',' >> ruleset.r_scalarExpression ) | qi::attr(ScalarPtr()) )
      >> ')' ) 
     [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Cylinder>(qi::_1, qi::_3, qi::_4, qi::_5, qi::_2)) ]
      
    ))
  );
}

}
}
