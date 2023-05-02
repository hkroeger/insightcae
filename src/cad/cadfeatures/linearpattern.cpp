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

    
    
    
defineType(LinearPattern);
//addToFactoryTable(Feature, LinearPattern);
addToStaticFunctionTable(Feature, LinearPattern, insertrule);
addToStaticFunctionTable(Feature, LinearPattern, ruleDocumentation);

size_t LinearPattern::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=*m1_;
  if (otherpat_)
    {
      h+=*otherpat_;
    }
  else
  {
      h+=axis_->value();
      h+=n_->value();
  }
  return h.getHash();
}



  
LinearPattern::LinearPattern(FeaturePtr m1, VectorPtr axis, ScalarPtr n)
: m1_(m1), axis_(axis), n_(n)
{}


LinearPattern::LinearPattern(FeaturePtr m1, FeaturePtr otherpat)
: m1_(m1), otherpat_(otherpat)
{}




void LinearPattern::build()
{
//   BRep_Builder bb;
//   TopoDS_Compound result;
//   bb.MakeCompound(result);

    double delta_x;
    int n;
    arma::mat ax0;
    if (otherpat_)
    {
        n=otherpat_->getDatumScalar("n");
        delta_x=otherpat_->getDatumScalar("delta_x");
        ax0=otherpat_->getDatumVector("axis");
    }
    else
    {
        delta_x=norm ( axis_->value(), 2 );
        ax0= axis_->value() /delta_x;
        n=round ( n_->value() );
    }
    gp_Vec ax ( to_Vec (ax0) );

    int j=0;
    CompoundFeatureMap instances;

    std::map<std::string, std::vector<FeaturePtr> > subshapeCompoundFeatures;
    auto sf = m1_->providedSubshapes();

    for ( int i=0; i<n; i++ ) {
        gp_Trsf tr;
        tr.SetTranslation ( ax*delta_x*double ( i ) );
//     bb.Add(result, BRepBuilderAPI_Transform(m1_->shape(), tr).Shape());

        components_[str ( format ( "component%d" ) % ( j+1 ) )] = Transform::create ( m1_, tr );
        j++;

        for (const auto& pss: sf)
        {
          subshapeCompoundFeatures[pss.first].push_back(Transform::create ( m1_->subshape(pss.first), tr ));
        }
    }

    refvalues_["n"]=n;
    refvalues_["delta_x"]=delta_x;
    refvectors_["axis"]=ax0;
    providedSubshapes_["basefeat"]=m1_;

    for (const auto& pss: sf)
    {
      providedSubshapes_[pss.first]=Compound::create(subshapeCompoundFeatures[pss.first]);
    }

    m1_->unsetLeaf();
    Compound::build();

//   setShape(result);
}




void LinearPattern::insertrule(parser::ISCADParser& ruleset)
{
  ruleset.modelstepFunctionRules.add
  (
    "LinearPattern",	
    typename parser::ISCADParser::ModelstepRulePtr(new typename parser::ISCADParser::ModelstepRule( 

    ( '(' >> ruleset.r_solidmodel_expression >> 
      ',' >> ruleset.r_vectorExpression >> 
      ',' >> ruleset.r_scalarExpression >> ')' ) 
      [ qi::_val = phx::bind(
                         &LinearPattern::create<FeaturePtr, VectorPtr, ScalarPtr>,
                         qi::_1, qi::_2, qi::_3) ]
    |
    (
     '(' >>
       ruleset.r_solidmodel_expression >> ',' >> ruleset.r_solidmodel_expression
      >> ')'
    ) [ qi::_val = phx::bind(
                          &LinearPattern::create<FeaturePtr, FeaturePtr>,
                          qi::_1, qi::_2) ]

    ))
  );
}




FeatureCmdInfoList LinearPattern::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "LinearPattern",
         
            "( <feature:base>, <vector:delta_l>, <scalar:n> )",
         
            "Copies the bease feature base into a linear pattern."
            " The copies of the base feature are shifted in increments of delta_l."
            " The number of copies is n."
        )
    };
}



}
}
