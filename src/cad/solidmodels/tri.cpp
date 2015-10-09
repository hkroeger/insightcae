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

#include "tri.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;


namespace insight {
namespace cad {

defineType(Tri);
addToFactoryTable(SolidModel, Tri, NoParameters);

Tri::Tri(const NoParameters&)
{}

Tri::Tri(const arma::mat& p0, const arma::mat& e1, const arma::mat& e2)
{
  gp_Pnt 
    p1(to_Pnt(p0)),
    p2=p1.Translated(to_Vec(e1)),
    p3=p1.Translated(to_Vec(e2))
  ;
  
  BRepBuilderAPI_MakeWire w;
  w.Add(BRepBuilderAPI_MakeEdge(p1, p2));
  w.Add(BRepBuilderAPI_MakeEdge(p2, p3));
  w.Add(BRepBuilderAPI_MakeEdge(p3, p1));
  
//   providedSubshapes_["OuterWire"].reset(new SolidModel(w.Wire()));
  providedSubshapes_.add("OuterWire", SolidModelPtr(new SolidModel(w.Wire())));
  
  setShape(BRepBuilderAPI_MakeFace(w.Wire()));
}

Tri::operator const TopoDS_Face& () const
{
  return TopoDS::Face(shape_);
}

void Tri::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Tri",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Tri>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

}
}
