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

#include "arc.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{
  
  
defineType(Arc);
addToFactoryTable(Feature, Arc, NoParameters);



Arc::Arc(const NoParameters& nop)
: Feature(nop)
{

}

void Arc::build()
{
  Handle_Geom_TrimmedCurve crv=GC_MakeArcOfCircle(to_Pnt(*p0_), to_Vec(*p0tang_), to_Pnt(*p1_));
  setShape(BRepBuilderAPI_MakeEdge(crv));
  
  gp_Pnt p;
  gp_Vec v;
  crv->D1(crv->FirstParameter(), p, v);
  refpoints_["p0"]=vec3(p);
  refvectors_["et0"]=vec3(v);
  crv->D1(crv->LastParameter(), p, v);
  refpoints_["p1"]=vec3(p);
  refvectors_["et1"]=vec3(v);
}


Arc::Arc(VectorPtr p0, VectorPtr p0tang, VectorPtr p1)
: p0_(p0), p0tang_(p0tang), p1_(p1)
{
}

void Arc::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Arc",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ',' >> ruleset.r_vectorExpression >> ')' ) 
	[ qi::_val = phx::construct<FeaturePtr>(phx::new_<Arc>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}

bool Arc::isSingleCloseWire() const
{
  return false;
}

bool Arc::isSingleOpenWire() const
{
  return true;
}

}
}
