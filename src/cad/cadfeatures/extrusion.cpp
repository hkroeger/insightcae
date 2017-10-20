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

#include "extrusion.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight {
namespace cad {


    
    
defineType(Extrusion);
addToFactoryTable(Feature, Extrusion);




Extrusion::Extrusion(): Feature()
{}




Extrusion::Extrusion(FeaturePtr sk, VectorPtr L, bool centered)
: sk_(sk), L_(L), centered_(centered)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*sk_;
  h+=L_->value();
  h+=centered_;
}




FeaturePtr Extrusion::create(FeaturePtr sk, VectorPtr L, bool centered)
{
    return FeaturePtr(new Extrusion(sk, L, centered));
}




void Extrusion::build()
{
    if ( !centered_ ) {
        BRepPrimAPI_MakePrism mkp ( sk_->shape(), to_Vec ( L_->value() ) );
        providedSubshapes_["frontFace"]=FeaturePtr ( new Feature ( mkp.FirstShape() ) );
        providedSubshapes_["backFace"]=FeaturePtr ( new Feature ( mkp.LastShape() ) );
        setShape ( mkp.Shape() );
    } else {
        gp_Trsf trsf;
        trsf.SetTranslation ( to_Vec ( -0.5*L_->value() ) );
        BRepPrimAPI_MakePrism mkp
        (
            BRepBuilderAPI_Transform ( sk_->shape(), trsf ).Shape(),
            to_Vec ( L_->value() )
        );
        providedSubshapes_["frontFace"]=FeaturePtr ( new Feature ( mkp.FirstShape() ) );
        providedSubshapes_["backFace"]=FeaturePtr ( new Feature ( mkp.LastShape() ) );
        setShape ( mkp.Shape() );
    }


    copyDatums ( *sk_ );
}




void Extrusion::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Extrusion",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> ',' >> ruleset.r_vectorExpression
      >> ( (  ',' >> qi::lit("centered") >> qi::attr(true) ) | qi::attr(false) ) 
      >> ')' )
      [ qi::_val = phx::bind(&Extrusion::create, qi::_1, qi::_2, qi::_3) ]
      
    ))
  );
}




FeatureCmdInfoList Extrusion::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Extrusion",
         
            "( <feature:sec>, <vector:L> [, centered] )",
         
            "Creates an extrusion of the planar feature sec. The direction and length of the extrusion is given by the vector L."
            " If the keyword centered is given, the extrusion is centered around the base feature."
        )
    );
}



}
}
