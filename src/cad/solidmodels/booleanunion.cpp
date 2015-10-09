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

#include "booleanunion.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(BooleanUnion);
addToFactoryTable(SolidModel, BooleanUnion, NoParameters);

BooleanUnion::BooleanUnion(const NoParameters& nop): SolidModel(nop)
{}

BooleanUnion::BooleanUnion(const SolidModel& m)
{
  m.unsetLeaf();
  
  TopoDS_Shape res;
  for (TopExp_Explorer ex(m, TopAbs_SOLID); ex.More(); ex.Next())
  {
    if (res.IsNull())
      res=TopoDS::Solid(ex.Current());
    else
      res=BRepAlgoAPI_Fuse(res, TopoDS::Solid(ex.Current())).Shape();
  }
  setShape(res);
}

BooleanUnion::BooleanUnion(const SolidModel& m1, const SolidModel& m2)
: SolidModel(BRepAlgoAPI_Fuse(m1, m2).Shape())
{
  m1.unsetLeaf();
  m2.unsetLeaf();
}


void BooleanUnion::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "MergeSolids",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<BooleanUnion>(*qi::_1)) ]
      
    ))
  );
}

SolidModel operator|(const SolidModel& m1, const SolidModel& m2)
{
  return BooleanUnion(m1, m2);
}


}
}