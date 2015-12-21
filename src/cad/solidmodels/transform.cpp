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

#include "transform.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


defineType(Transform);
addToFactoryTable(SolidModel, Transform, NoParameters);

Transform::Transform(const NoParameters& nop): SolidModel(nop)
{}


TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale)
{
  gp_Trsf tr0, tr1, tr2;
//   TopoDS_Shape intermediate_shape=m1;
//   
  tr0.SetScaleFactor(scale);
//   intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr0).Shape();
// 
  tr1.SetTranslation(to_Vec(trans));  
//   intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr1).Shape();
// 
  double phi=norm(rot, 2);
  if (phi>1e-10)
  {
    gp_Vec axis=to_Vec(rot);
    axis.Normalize();
    tr2.SetRotation(gp_Ax1(gp_Pnt(0,0,0), axis), phi);
//     intermediate_shape=BRepBuilderAPI_Transform(intermediate_shape, tr2).Shape();
  }  
  
  gp_Trsf trcomp=tr2.Multiplied(tr1).Multiplied(tr0);
  return makeTransform(m1, trcomp);
// 
//   // Apply rotation first, then translation
//   return intermediate_shape;
}

TopoDS_Shape Transform::makeTransform(const SolidModel& m1, const gp_Trsf& trsf)
{
  if (m1.hasExplicitCoG())
  {
    this->setCoGExplicitly( vec3(to_Pnt(m1.modelCoG()).Transformed(trsf)) );
  }
  if (m1.hasExplicitMass()) setMassExplicitly(m1.mass());
  
  // Transform all ref points and ref vectors
  copyDatumsTransformed(m1, trsf);
  
  return BRepBuilderAPI_Transform(m1, trsf).Shape();
}


Transform::Transform(const SolidModel& m1, const arma::mat& trans, const arma::mat& rot, double scale)
{
  setShape(makeTransform(m1, trans, rot, scale));
  m1.unsetLeaf();
}

Transform::Transform(const SolidModel& m1, const arma::mat& trans)
{
  setShape(makeTransform(m1, trans, vec3(0,0,0), 1.0));
  m1.unsetLeaf();
}

Transform::Transform(const SolidModel& m1, double sf)
{
  setShape(makeTransform(m1, vec3(0,0,0), vec3(0,0,0), sf));
  m1.unsetLeaf();
}

Transform::Transform(const SolidModel& m1, const gp_Trsf& trsf)
{
  setShape(makeTransform(m1, trsf));
  m1.unsetLeaf();
}

void Transform::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Transform",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	> ruleset.r_vectorExpression > ( (',' > ruleset.r_scalarExpression ) | qi::attr(1.0) ) > ')' ) 
      [ qi::_val = phx::construct<SolidModelPtr>(phx::new_<Transform>(*qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
}


}
}
