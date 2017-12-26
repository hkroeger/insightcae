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

#include "booleansubtract.h"
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


    
    
defineType(BooleanSubtract);
addToFactoryTable(Feature, BooleanSubtract);




BooleanSubtract::BooleanSubtract()
: DerivedFeature()
{}




BooleanSubtract::BooleanSubtract(FeaturePtr m1, FeaturePtr m2)
: DerivedFeature(m1),
  m1_(m1),
  m2_(m2)
{
  ParameterListHash h(this);
  h+=this->type();
  h+=*m1_;
  h+=*m2_;
  setFeatureSymbolName( "("+m1->featureSymbolName()+" - "+m2->featureSymbolName()+")" );
}




FeaturePtr BooleanSubtract::create(FeaturePtr m1, FeaturePtr m2)
{
    return FeaturePtr(new BooleanSubtract(m1, m2));
}




void BooleanSubtract::build()
{
  
  if (!cache.contains(hash()))
  {
    if (!m1_) throw insight::cad::CADException(*this, "invalid input: base shape" );
    if (!m2_) throw insight::cad::CADException(*this, "invalid input: tool shape" );
    BRepAlgoAPI_Cut cutter(*m1_, *m2_);
    cutter.Build();
    if (!cutter.IsDone())
    {
        throw insight::cad::CADException
        (
            *this, 
            "could not perform cut operation."
        );
    }
    TopoDS_Shape subs=cutter.Shape();
    setShape(subs);
  
    copyDatums(*m1_);
    cache.insert(shared_from_this());
  }
  else
  {
      this->operator=(*cache.markAsUsed<BooleanSubtract>(hash()));
  }
  m1_->unsetLeaf();
  m2_->unsetLeaf();
}


void BooleanSubtract::operator=(const BooleanSubtract& o)
{
    m1_=o.m1_;
    m2_=o.m2_;
    Feature::operator=(o);
}


FeaturePtr operator-(FeaturePtr m1, FeaturePtr m2)
{
  return BooleanSubtract::create(m1, m2);
}




/*! \page BooleanSubtract BooleanSubtract
  * Return the subtraction of feat2 from feat1.
  * 
  * Syntax: 
  * ~~~~
  * ( <feature expression: feat1> - <feature expression: feat2> ) : feature
  * ~~~~
  */
void BooleanSubtract::insertrule(parser::ISCADParser& ruleset) const
{
//   ruleset.modelstepFunctionRules.add
//   (
//     "",	
//     typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 
// 
//     
//       
//     ))
//   );
}




FeatureCmdInfoList BooleanSubtract::ruleDocumentation() const
{
    return FeatureCmdInfoList();
}




}
}
