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

namespace insight 
{
namespace cad 
{


defineType(Transform);
addToFactoryTable(Feature, Transform, NoParameters);

Transform::Transform(const NoParameters& nop)
: DerivedFeature(nop)
{}




Transform::Transform(FeaturePtr m1, VectorPtr trans, VectorPtr rot, ScalarPtr scale)
: DerivedFeature(m1),
  m1_(m1),
  trans_(trans),
  rot_(rot),
  sf_(scale)
{
}




Transform::Transform(FeaturePtr m1, VectorPtr rot, VectorPtr rotorg)
: DerivedFeature(m1),
  m1_(m1),
  rot_(rot),
  rotorg_(rotorg)
{
}




Transform::Transform(FeaturePtr m1, VectorPtr trans)
: DerivedFeature(m1),
  m1_(m1),
  trans_(trans)
{
}




Transform::Transform(FeaturePtr m1, ScalarPtr sf)
: DerivedFeature(m1),
  m1_(m1),
  sf_(sf)
{
}




Transform::Transform(FeaturePtr m1, const gp_Trsf& trsf)
: DerivedFeature(m1),
  m1_(m1), trsf_(new gp_Trsf(trsf))
{
}




void Transform::build()
{
    gp_Trsf tr0, tr1, tr2;

    if (!trsf_)
    {
        if (sf_)
            tr0.SetScaleFactor(*sf_);

        if (trans_)
            tr1.SetTranslation(to_Vec(*trans_));

        if (rot_)
        {
            double phi=norm(rot_->value(), 2);
            if (phi>1e-10)
            {
                gp_Vec axis=to_Vec(rot_->value());
                axis.Normalize();
                gp_Pnt rorg(0,0,0);
                if (rotorg_) rorg=to_Pnt(rotorg_->value());
                tr2.SetRotation(gp_Ax1(rorg, axis), phi);
            }
        }

        trsf_.reset(new gp_Trsf(tr2.Multiplied(tr1).Multiplied(tr0)));
    }

    setShape(BRepBuilderAPI_Transform(*m1_, *trsf_).Shape());

    // Transform all ref points and ref vectors
    copyDatumsTransformed(*m1_, *trsf_);
}




bool Transform::isTransformationFeature() const 
{
 if (sf_)
 {
  if (fabs(sf_->value()-1.0)>1e-6) 
  {
   return false;
  }
 } 
 return true; 
}




void Transform::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Transform",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	> ruleset.r_vectorExpression > 
	( 
	 (',' > ruleset.r_scalarExpression ) 
	 | 
	 qi::attr(ScalarPtr( new ConstantScalar(1.0)))
	) > ')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Transform>(qi::_1, qi::_2, qi::_3, qi::_4)) ]
      
    ))
  );
  
  ruleset.modelstepFunctionRules.add
  (
    "Rotate",
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_vectorExpression > ',' 
	> ruleset.r_vectorExpression > 
	')' ) 
      [ qi::_val = phx::construct<FeaturePtr>(phx::new_<Transform>(qi::_1, qi::_2, qi::_3)) ]
      
    ))
  );
}




gp_Trsf Transform::transformation() const
{
  checkForBuildDuringAccess();
  return *trsf_;
}


}
}
