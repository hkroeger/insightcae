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

namespace insight 
{
namespace cad 
{


    
    
defineType(BooleanUnion);
addToFactoryTable(Feature, BooleanUnion);




BooleanUnion::BooleanUnion()
: DerivedFeature()
{}




BooleanUnion::BooleanUnion(FeaturePtr m1)
: DerivedFeature(m1), 
  m1_(m1)
{}




BooleanUnion::BooleanUnion(FeaturePtr m1, FeaturePtr m2)
: DerivedFeature(m1),
  m1_(m1),
  m2_(m2)
{}




FeaturePtr BooleanUnion::create(FeaturePtr m1)
{
    return FeaturePtr(new BooleanUnion(m1));
}




FeaturePtr BooleanUnion::create(FeaturePtr m1, FeaturePtr m2)
{
    return FeaturePtr(new BooleanUnion(m1, m2));
}
     
     
     
     
void BooleanUnion::build()
{
    if (m1_ && m2_)
    {
        ParameterListHash h(this);
        h+=*m1_;
        h+=*m2_;

        copyDatums(*m1_, "m1_");
        copyDatums(*m2_, "m2_");
        m1_->unsetLeaf();
        m2_->unsetLeaf();
        setShape(BRepAlgoAPI_Fuse(*m1_, *m2_).Shape());
    }
    else
    {
        ParameterListHash h(this);
        h+=*m1_;

        copyDatums(*m1_);
        m1_->unsetLeaf();

        TopoDS_Shape res;
        for (TopExp_Explorer ex(*m1_, TopAbs_SOLID); ex.More(); ex.Next())
        {
            if (res.IsNull())
                res=TopoDS::Solid(ex.Current());
            else
                res=BRepAlgoAPI_Fuse(res, TopoDS::Solid(ex.Current())).Shape();
        }
        setShape(res);
    }
}




/*! \page BooleanUnion BooleanUnion
  * Return the union of feat1 with feat2.
  * 
  * Syntax: 
  * ~~~~
  * ( <feature expression: feat1> | <feature expression: feat2> ) : feature
  * ~~~~
  * 
  * MergeSolids: Return the union of all solids in feat.
  * 
  * Syntax: 
  * ~~~~
  * MergeSolids( <feature expression: feat> ) : feature
  * ~~~~
  */
void BooleanUnion::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "MergeSolids",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ')' ) 
      [ qi::_val = phx::bind(&BooleanUnion::create, qi::_1) ]
      
    ))
  );
}





FeatureCmdInfoList BooleanUnion::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "MergeSolids",
         
            "( <feature> )",
         
            "Creates a boolean union of all (possibly intersecting) volumes of the given feature."
        )
    );
}




FeaturePtr operator|(FeaturePtr m1, FeaturePtr m2)
{
  return BooleanUnion::create(m1, m2);
}




}
}
