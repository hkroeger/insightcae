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

#include "line.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Line);
addToFactoryTable(SolidModel, Line, NoParameters);


Line::Line(const NoParameters& nop)
: SolidModel(nop)
{
}

Line::Line(const arma::mat& p0, const arma::mat& p1)
: SolidModel()
{
  setShape(BRepBuilderAPI_MakeEdge(GC_MakeSegment(to_Pnt(p0), to_Pnt(p1))));
}

void Line::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Line",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Line>(qi::_1, qi::_2)) ]
      
    ))
  );
}

bool Line::isSingleCloseWire() const
{
  return false;
}

bool Line::isSingleOpenWire() const
{
  return true;
}

}
}
