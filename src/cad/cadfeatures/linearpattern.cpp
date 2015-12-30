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

#include "linearpattern.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

defineType(LinearPattern);
addToFactoryTable(SolidModel, LinearPattern, NoParameters);

LinearPattern::LinearPattern(const NoParameters& nop): SolidModel(nop)
{}


  
LinearPattern::LinearPattern(const SolidModel& m1, const arma::mat& axis, int n)
{
  BRep_Builder bb;
  TopoDS_Compound result;
  bb.MakeCompound(result);
  
  double delta_x=norm(axis, 2);
  gp_Vec ax(to_Vec(axis/delta_x));
  
  for (int i=0; i<n; i++)
  {
    gp_Trsf tr;
    tr.SetTranslation(ax*delta_x*double(i));
    bb.Add(result, BRepBuilderAPI_Transform(m1, tr).Shape());
  }
  m1.unsetLeaf();
  
  setShape(result);
}

void LinearPattern::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "LinearPattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > 
      ',' > ruleset.r_vectorExpression > 
      ',' > ruleset.r_scalarExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<LinearPattern>(*qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}


}
}