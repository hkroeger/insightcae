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

#include "revolution.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {

  
defineType(Revolution);
addToFactoryTable(Feature, Revolution, NoParameters);

Revolution::Revolution(const NoParameters& nop): Feature(nop)
{}


TopoDS_Shape makeRevolution(const Feature& sk, const arma::mat& p0, const arma::mat& axis, double ang, bool centered)
{
  if (!centered)
  {
    return BRepPrimAPI_MakeRevol( sk, gp_Ax1(to_Pnt(p0), gp_Dir(to_Vec(axis))), ang, centered ).Shape();
  }
  else
  {
    gp_Trsf trsf;
    gp_Vec ax=to_Vec(axis);
    ax.Normalize();
    trsf.SetRotation(gp_Ax1(to_Pnt(p0), ax), -0.5*ang);
    return BRepPrimAPI_MakeRevol
    ( 
      BRepBuilderAPI_Transform(sk, trsf).Shape(), 
      gp_Ax1(to_Pnt(p0), gp_Dir(ax)), ang
    ).Shape();
  }
}

Revolution::Revolution(FeaturePtr sk, VectorPtr p0, VectorPtr axis, ScalarPtr angle, bool centered)
: sk_(sk), p0_(p0), axis_(axis), angle_(angle), centered_(centered)
{}

void Revolution::build()
{
  setShape(makeRevolution(*sk_, p0_->value(), axis_->value(), angle_->value(), centered_));
}

void Revolution::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Revolution",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	  > ruleset.r_vectorExpression > ',' > ruleset.r_scalarExpression 
       > ( (  ',' > qi::lit("centered") > qi::attr(true) ) | qi::attr(false))
       > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Revolution>(qi::_1, qi::_2, qi::_3, qi::_4, qi::_5)) ]
      
    ))
  );
}

}
}