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

#include "ring.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Ring);
addToFactoryTable(SolidModel, Ring, NoParameters);

Ring::Ring(const NoParameters&)
{}


Ring::Ring(const arma::mat& p1, const arma::mat& p2, double Da, double Di)
{
  setShape
  (
    BRepAlgoAPI_Cut
    (
      
      BRepPrimAPI_MakeCylinder
      (
	gp_Ax2
	(
	  gp_Pnt(p1(0),p1(1),p1(2)), 
	  gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
	),
	0.5*Da, 
	norm(p2-p1, 2)
      ).Shape(),
     
      BRepPrimAPI_MakeCylinder
      (
	gp_Ax2
	(
	  gp_Pnt(p1(0),p1(1),p1(2)), 
	  gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
	),
	0.5*Di, 
	norm(p2-p1, 2)
      ).Shape()
      
    ).Shape()   
  );
}

void Ring::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Ring",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	  [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Ring>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}
 
}
}
