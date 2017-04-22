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
addToFactoryTable(Feature, Place);




Place::Place(): DerivedFeature()
{}




Place::Place(FeaturePtr m, const gp_Ax2& cs)
: DerivedFeature(m), m_(m) 
{
  trsf_.reset(new gp_Trsf);
  trsf_->SetTransformation(gp_Ax3(cs));
  
  trsf_->Invert();
//   makePlacement(m, tr.Inverted());
}




Place::Place(FeaturePtr m, VectorPtr p0, VectorPtr ex, VectorPtr ez)
: DerivedFeature(m), m_(m), p0_(p0), ex_(ex), ez_(ez)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m_;
  h+=p0_->value();
  h+=ex_->value();
  h+=ez_->value();
}




Place::Place(FeaturePtr m, FeaturePtr other)
: DerivedFeature(m), m_(m), other_(other)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m_;
  h+=*other_;
}




FeaturePtr Place::create(FeaturePtr m, VectorPtr p0, VectorPtr ex, VectorPtr ez)
{
    return FeaturePtr(new Place(m, p0, ex, ez));
}




FeaturePtr Place::create(FeaturePtr m, FeaturePtr other)
{
    return FeaturePtr(new Place(m, other));
}




void Place::build()
{
  if (!trsf_)
  {
      
    trsf_.reset(new gp_Trsf);
    
    if (other_)
    {
        trsf_->SetTransformation
        (
            gp_Ax3(to_Pnt(other_->getDatumPoint("origin")), 
            to_Vec(other_->getDatumVector("ez")), 
            to_Vec(other_->getDatumVector("ex")))
        );
    }
    else
    {
        arma::mat p0=p0_->value(), ez=ez_->value(), ex=ex_->value();
        trsf_->SetTransformation(gp_Ax3(to_Pnt(p0), to_Vec(ez), to_Vec(ex)));
        refpoints_["origin"]=p0;
        refvectors_["ex"]=ex;
        refvectors_["ez"]=ez;
    }
    
    trsf_->Invert();
  //   makePlacement(m, tr.Inverted());
  }
 
  setShape(BRepBuilderAPI_Transform(m_->shape(), *trsf_).Shape());
  
  copyDatumsTransformed( *m_, *trsf_, "", boost::assign::list_of("origin")("ex")("ez") );
}




void Place::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Place",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' 
       >> ruleset.r_solidmodel_expression >> ',' 
       >> ruleset.r_vectorExpression >> ',' 
       >> ruleset.r_vectorExpression >> ',' 
       >> ruleset.r_vectorExpression 
       >> ')' ) 
      [ qi::_val = phx::bind(&Place::create, qi::_1, qi::_2, qi::_3, qi::_4) ]
    |
    ( '(' 
       >> ruleset.r_solidmodel_expression >> ',' 
       >> ruleset.r_solidmodel_expression 
       >> ')' ) 
      [ qi::_val = phx::bind(&Place::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Place::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Place",
            "( <feature:base>, ( <vector:p0>, <vector:ex>, <vector:ez> ) | <feature:other_place> )",
            "Places the feature base in a new coordinate system. The new origin is at point p0, the new x-axis along vector ex and the new z-direction is ez."
            " Alternatively, the placement is copied from another Place-feature other_place."
        )
    );
}



gp_Trsf Place::transformation() const
{
  checkForBuildDuringAccess();
  return *trsf_;
}




}
}
