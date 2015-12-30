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

#include "place.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Place);
addToFactoryTable(SolidModel, Place, NoParameters);

void Place::makePlacement(const SolidModel& m, const gp_Trsf& tr)
{
  if (m.hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m.modelCoG()).Transformed(tr)) );
  }
  if (m.hasExplicitMass()) setMassExplicitly(m.mass());
  setShape(BRepBuilderAPI_Transform(m, tr).Shape());
  
  copyDatumsTransformed(m, tr);
}


Place::Place(const NoParameters& nop): SolidModel(nop)
{}


Place::Place(const SolidModel& m, const gp_Ax2& cs)
{
  gp_Trsf tr;
  tr.SetTransformation(gp_Ax3(cs));
  makePlacement(m, tr.Inverted());
}


Place::Place(const SolidModel& m, const arma::mat& p0, const arma::mat& ex, const arma::mat& ez)
{
  gp_Trsf tr;
  tr.SetTransformation(gp_Ax3(to_Pnt(p0), to_Vec(ez), to_Vec(ex)));
  makePlacement(m, tr.Inverted());
}

void Place::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Place",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > 
	  ',' > ruleset.r_vectorExpression > ',' > ruleset.r_vectorExpression > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Place>(*qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}


}
}
