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
addToFactoryTable(SolidModel, Cylinder, NoParameters);

Cylinder::Cylinder(const NoParameters&)
{}


Cylinder::Cylinder(const arma::mat& p1, const arma::mat& p2, double D)
{
  setShape
  (
    BRepPrimAPI_MakeCylinder
    (
      gp_Ax2
      (
	gp_Pnt(p1(0),p1(1),p1(2)), 
	gp_Dir(p2(0)-p1(0),p2(1)-p1(1),p2(2)-p1(2))
      ),
      0.5*D, 
      norm(p2-p1, 2)
    ).Shape()
  );
}

void Cylinder::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Cylinder",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression > ')' ) 
	  [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Cylinder>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
