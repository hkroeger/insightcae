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

#include "cone.h"
#include "base/boost_include.h"

#include "BRepPrimAPI_MakeCone.hxx"

#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(Cone);
addToFactoryTable(Feature, Cone, NoParameters);

Cone::Cone(const NoParameters&)
{}

Cone::Cone(VectorPtr p1, VectorPtr p2, ScalarPtr D1, ScalarPtr D2)
: p1_(p1), p2_(p2), D1_(D1), D2_(D2)
{}

void Cone::build()
{
  TopoDS_Shape cone=
    BRepPrimAPI_MakeCone
    (
      gp_Ax2
      (
	gp_Pnt(p1_->value()(0),p1_->value()(1),p1_->value()(2)), 
	gp_Dir(p2_->value()(0)-p1_->value()(0),p2_->value()(1)-p1_->value()(1),p2_->value()(2)-p1_->value()(2))
      ),
      0.5*D1_->value(), 
      0.5*D2_->value(), 
      norm(p2_->value()-p1_->value(), 2)
    ).Shape();
    
//   if (Di_)
//   {
//     cyl=BRepAlgoAPI_Cut
//     (
//       
//       cyl,
//      
//       BRepPrimAPI_MakeCylinder
//       (
// 	gp_Ax2
// 	(
// 	  gp_Pnt(p1_->value()(0),p1_->value()(1),p1_->value()(2)), 
// 	  gp_Dir(p2_->value()(0)-p1_->value()(0),p2_->value()(1)-p1_->value()(1),p2_->value()(2)-p1_->value()(2))
// 	),
// 	0.5*Di_->value(), 
// 	norm(p2_->value()-p1_->value(), 2)
//       ).Shape()
//       
//     );
//   }

  setShape(cone);
}

void Cone::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cone",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_vectorExpression >> ',' 
      >> ruleset.r_scalarExpression >> ','
      >> ruleset.r_scalarExpression
      >> ')' ) 
     [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Cone>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}

}
}