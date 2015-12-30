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

#include "regpoly.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(RegPoly);
addToFactoryTable(SolidModel, RegPoly, NoParameters);

RegPoly::RegPoly(const NoParameters&)
{}


RegPoly::RegPoly(const arma::mat& p0, const arma::mat& n, double ne, double a, 
		 const arma::mat& ez)
{
  double ru=a/::cos(M_PI/ne);
  arma::mat e0=ez;
  if (e0.n_elem==0)
  {
    e0=cross(n, vec3(1,0,0));
    if (norm(e0,2)<1e-6) 
      e0=cross(n, vec3(0,1,0));
  }
  int z=round(ne);
  double delta_phi=2.*M_PI/double(z);
  BRepBuilderAPI_MakeWire w;
  for (int i=0; i<z; i++)
  {
    arma::mat npm=p0+rotMatrix((0.5+double(i-1))*delta_phi, n)*(ru*e0);
    arma::mat np=p0+rotMatrix((0.5+double(i))*delta_phi, n)*(ru*e0);
    w.Add(BRepBuilderAPI_MakeEdge(to_Pnt(npm), to_Pnt(np)));
  }
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_.add("OuterWire", SolidModelPtr(new SolidModel(w.Wire())));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}

void RegPoly::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "RegPoly",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression 
			      > ',' > ruleset.r_scalarExpression > ',' > ruleset.r_scalarExpression 
			      > ( (',' > ruleset.r_vectorExpression)|qi::attr(arma::mat()) ) > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<RegPoly>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

}
}
