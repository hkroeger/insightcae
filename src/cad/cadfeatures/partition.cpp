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

#include "partition.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>

#include "BOPAlgo_Builder.hxx"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


    
    
defineType(Partition);
addToFactoryTable(Feature, Partition);




Partition::Partition()
: DerivedFeature()
{}




Partition::Partition(FeaturePtr m1, FeaturePtr m2)
: DerivedFeature(m1),
  m1_(m1),
  m2_(m2)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m1_;
  h+=*m2_;
}




FeaturePtr Partition::create(FeaturePtr m1, FeaturePtr m2)
{
    return FeaturePtr(new Partition(m1, m2));
}




void Partition::build()
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m1_;
  h+=*m2_;

  BOPAlgo_Builder bab;
  bab.AddArgument(*m1_);
  bab.AddArgument(*m2_);
  setShape(bab.Shape());
  
  copyDatums(*m1_);
  m1_->unsetLeaf();
  m2_->unsetLeaf();
}









void Partition::insertrule(parser::ISCADParser& ruleset) const
{
  ruleset.modelstepFunctionRules.add
  (
    "Partition",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' > ruleset.r_solidmodel_expression > ',' > ruleset.r_solidmodel_expression > ')' )
	[ qi::_val = phx::bind(&Partition::create, qi::_1, qi::_2) ]
      
    ))
  );
}




FeatureCmdInfoList Partition::ruleDocumentation() const
{
    return boost::assign::list_of
    (
        FeatureCmdInfo
        (
            "Partition",
         
            " (<base feature>, <tool feature>] )",
         
            "Partionates of the base feature by the tool feature."
        )
    );
}




}
}
