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



#include "line.h"
#include "datum.h"
#include "base/boost_include.h"
#include <boost/spirit/include/qi.hpp>
#include "base/translations.h"

#include "constrainedsketch.h"

namespace qi = boost::spirit::qi;
namespace repo = boost::spirit::repository;
namespace phx   = boost::phoenix;

using namespace std;
using namespace boost;

namespace insight 
{
namespace cad 
{


    
    
defineType(Line);
//addToFactoryTable(Feature, Line);
addToStaticFunctionTable(Feature, Line, insertrule);
addToStaticFunctionTable(Feature, Line, ruleDocumentation);

addToStaticFunctionTable(ConstrainedSketchEntity, Line, addParserRule);


size_t Line::calcHash() const
{
  ParameterListHash h;
  h+=this->type();
  h+=p0_->value();
  h+=p1_->value();
  h+=second_is_dir_;
  return h.getHash();
}



Line::Line(VectorPtr p0, VectorPtr p1, bool second_is_dir)
: p0_(p0), p1_(p1),
  second_is_dir_(second_is_dir)
{}


VectorPtr Line::start() const
{
    return p0_;
}

VectorPtr Line::end() const
{
    return p1_;
}

void Line::scaleSketch(double scaleFactor)
{}



void Line::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity*, int> &entityLabels ) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "("
            + lexical_cast<std::string>(myLabel)+", "
            + pointSpec(p0_, script, entityLabels)
            + ", "
            + pointSpec(p1_, script, entityLabels)
            + parameterString()
            + ")"
        );
}

void Line::addParserRule(ConstrainedSketchGrammar &ruleset, MakeDefaultGeometryParametersFunction mdpf)
{
    namespace qi=boost::spirit::qi;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ',' > ruleset.r_point
             > ruleset.r_parameters > ')'
            )
            [   qi::_a = phx::bind(
                        &Line::create<VectorPtr, VectorPtr, bool>, qi::_2, qi::_3, false),
                phx::bind(&ConstrainedSketchEntity::changeDefaultParameters, qi::_a, phx::bind(mdpf)),
                phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4, boost::filesystem::path(".")),
                qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
        );
}



std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > Line::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret;

    if (auto sp1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p0_))
        ret.insert(sp1);
    if (auto sp2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_))
        ret.insert(sp2);

    return ret;
}

void Line::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto p = std::dynamic_pointer_cast<Vector>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p0_) == entity )
        {
            p0_ = p;
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_) == entity )
        {
            p1_ = p;
        }
    }
}




void Line::build()
{
  arma::mat p0=p0_->value();
  arma::mat p1;
  VectorPtr dir;
  if (second_is_dir_)
  {
    p1=p0+p1_->value();
    dir=p1_;
  }
  else
  {
    p1=p1_->value();
    dir=std::make_shared<SubtractedVector>(p1_, p0_);
  }

//  refpoints_["p0"]=p0;
//  refpoints_["p1"]=p1;
//   refvalues_["L"]=arma::norm(p1-p0, 2);
  refvectors_["ex"]=(p1 - p0)/arma::norm(p1 - p0, 2);


  providedDatums_["axis"]=
      std::make_shared<ExplicitDatumAxis>(
        p0_,
        dir
        );
  
  setShape(BRepBuilderAPI_MakeEdge(
      GC_MakeSegment(to_Pnt(p0), to_Pnt(p1)).Value()
  ));
}





void Line::insertrule(parser::ISCADParser& ruleset)
{
  typedef
    qi::rule<
            std::string::iterator,
            FeaturePtr(),
            parser::ISCADParser::skipper_type,
            qi::locals<VectorPtr>
            >

            LineRule;

  auto *rule = new LineRule(
      '(' > ruleset.r_vectorExpression[qi::_a=qi::_1] > ',' > (
      ruleset.r_vectorExpression
              [ qi::_val = phx::bind(&Line::create<VectorPtr, VectorPtr, bool>, qi::_a, qi::_1, false) ]
      |
      ( (qi::lit("dir")|qi::lit("direction")) > ruleset.r_vectorExpression )
              [ qi::_val = phx::bind(&Line::create<VectorPtr, VectorPtr, bool>, qi::_a, qi::_1, true) ]
       ) > ')'
  );
  ruleset.addAdditionalRule(rule);

  ruleset.modelstepFunctionRules.add
  (
    "Line",	
    std::make_shared<parser::ISCADParser::ModelstepRule>( *rule )
  );
}




FeatureCmdInfoList Line::ruleDocumentation()
{
    return {
        FeatureCmdInfo
        (
            "Line",
         
            "( <vector:p0>, <vector:p1> )",

          _("Creates a line between point p0 and p1.")
        )
    };
}



void Line::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const Line&>(other));
}


void Line::operator=(const Line& other)
{
    p0_=other.p0_;
    p1_=other.p1_;
    second_is_dir_=other.second_is_dir_;
    SingleEdgeFeature::operator=(other);
    ConstrainedSketchEntity::operator=(other);
}


}
}
