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
#include "base/exception.h"
#include "boost/phoenix/core/reference.hpp"
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




Line::Line(VectorPtr p0, VectorPtr p1, bool second_is_dir, const std::string& layerName)
  : ConstrainedSketchEntity(layerName),
    p0_(p0), p1_(p1),
    second_is_dir_(second_is_dir)
{}




VectorPtr Line::start() const
{
    return p0_;
}




VectorPtr Line::end() const
{
    if (second_is_dir_)
        return std::make_shared<cad::AddedVector>(p0_, p1_);
    else
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
            + lexical_cast<std::string>(myLabel)
            +", "
            + pointSpec(p0_, script, entityLabels)
            + ", "
            + pointSpec(p1_, script, entityLabels)
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void Line::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    namespace qi=boost::spirit::qi;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ','
             > ruleset.r_point
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters > ')'
            )
            [   qi::_a = phx::bind(
                 &Line::create<VectorPtr, VectorPtr, bool, const std::string&>,
                    qi::_2, qi::_3, false, qi::_4),
                phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters,
                       &pd, phx::ref(*qi::_a) ),
                phx::bind(&ConstrainedSketchEntity::parseParameterSet,
                       qi::_a, qi::_5, boost::filesystem::path(".")),
                qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                        qi::_1, qi::_a) ]
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
            invalidate();
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_) == entity )
        {
            p1_ = p;
            invalidate();
        }
    }
}


bool Line::isInside( SelectionRect r) const
{
    return
        r.isInside(p0_->value())
           && r.isInside(p1_->value());
}

bool Line::pointIsOnLine(const arma::mat &p3d) const
{
    arma::mat L=p1_->value()-p0_->value();
    double ll=arma::norm(L,2);
    L=L/ll;
    arma::mat l=(p3d - p0_->value())/ll;
    double d=arma::dot(l, L);
    double n=arma::norm(arma::cross(l, L), 2);
    std::cout<<"check d="<<d<<", n="<<n<<std::endl;
    return (d<=1.) && (d>=-insight::SMALL) && (fabs(n)<insight::SMALL);
}


arma::mat Line::projectOntoLine(const arma::mat &p) const
{
    arma::mat AB = end()->value() - start()->value();
    arma::mat AC = p - start()->value();
    arma::mat AD = AB* arma::dot(AB,AC)/arma::dot(AB, AB);
    return start()->value() + AD;
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

  double L=arma::norm(p1 - p0, 2);

  insight::assertion(L>0., "attempt to create a degenerated line with zero length");

  refvalues_["L"]=L;
//  refpoints_["p0"]=p0;
//  refpoints_["p1"]=p1;
//   refvalues_["L"]=arma::norm(p1-p0, 2);
  refvectors_["ex"]=(p1 - p0)/L;


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




ConstrainedSketchEntityPtr Line::clone() const
{
    auto cl=Line::create(
        p0_, p1_,
        second_is_dir_,
        layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}



std::vector<vtkSmartPointer<vtkProp> > Line::createActor() const
{
    return Feature::createVTKActors();
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
