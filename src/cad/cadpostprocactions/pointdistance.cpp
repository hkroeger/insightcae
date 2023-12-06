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

#include "cadfeature.h"
#include "pointdistance.h"

#include "AIS_Point.hxx"
// #include "AIS_Drawer.hxx"
#include "Prs3d_TextAspect.hxx"
#include "occtools.h"

#include "base/parameters/simpleparameter.h"
#include "constrainedsketch.h"
#include "parser.h"

using namespace std;
using namespace boost;

namespace insight {
namespace cad {




defineType(Distance);




size_t Distance::calcHash() const
{
  ParameterListHash h;
  h+=p1_->value();
  h+=p2_->value();
  return h.getHash();
}




Distance::Distance(insight::cad::VectorPtr p1, insight::cad::VectorPtr p2)
: p1_(p1), p2_(p2)
{}




void insight::cad::Distance::build()
{
  arma::mat p1=p1_->value();
  arma::mat p2=p2_->value();

  distance_=arma::norm(p2-p1,2);
}




void insight::cad::Distance::write(ostream&) const
{}




void Distance::operator=(const Distance &other)
{
  p1_=other.p1_;
  p2_=other.p2_;
  distance_=other.distance_;
  PostprocAction::operator=(other);
}




arma::mat Distance::dimLineOffset() const
{
  arma::mat dir = p2_->value() - p1_->value();
  arma::mat n = arma::cross(dir, vec3(0,0,1));
  if (arma::norm(n,2)<SMALL)
      n=arma::cross(dir, vec3(0,1,0));
  return normalized(n)*0.025*arma::norm(dir,2);
}




double Distance::relativeArrowSize() const
{
  return 0.025;
}








defineType(DistanceConstraint);




size_t DistanceConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=targetValue();
    return h.getHash();
}




DistanceConstraint::DistanceConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr planeNormal,
    const std::string& layerName)
    : ConstrainedSketchEntity(layerName),
    Distance(p1, p2),
    planeNormal_(planeNormal)
{
    changeDefaultParameters(
        ParameterSet({
            {"dimLineOfs", std::make_shared<DoubleParameter>(1., "dimension line offset")},
            {"arrowSize", std::make_shared<DoubleParameter>(1., "arrow size")}
        })
    );
}




int DistanceConstraint::nConstraints() const
{
    return 1;
}




double DistanceConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
                iConstraint==0,
                "invalid constraint id" );
    checkForBuildDuringAccess();
    return (distance_ - targetValue());
}




std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > DistanceConstraint::dependencies() const
{
    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > ret;

    if (auto sp1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_))
        ret.insert(sp1);
    if (auto sp2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(p2_))
        ret.insert(sp2);

    return ret;
}




void DistanceConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto p = std::dynamic_pointer_cast<Vector>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p1_) == entity)
        {
            p1_ = p;
        }
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p2_) == entity)
        {
            p2_ = p;
        }
    }
}




arma::mat DistanceConstraint::dimLineOffset() const
{
    arma::mat dir = p2_->value() - p1_->value();
    arma::mat n = normalized(arma::cross(dir, planeNormal_->value()));
    return n*parameters().getDouble("dimLineOfs");
}




void DistanceConstraint::setDimLineOffset(const arma::mat &p)
{
    arma::mat dir = p2_->value() - p1_->value();
    arma::mat dir2 = p - p1_->value();
    arma::mat n = normalized(arma::cross(dir, planeNormal_->value()));

    gp_Lin lin(
        to_Pnt(p1_->value()),
        to_Vec(dir)
        );

    auto &op = parametersRef().get<DoubleParameter>("dimLineOfs");
    op.set(
        lin.Distance(to_Pnt(p))
        * (arma::dot(n,dir2)<0.?-1.:1.),
        true
        );
}




double DistanceConstraint::relativeArrowSize() const
{
    double L = arma::norm(p2_->value() - p1_->value(), 2);
    return parameters().getDouble("arrowSize")/L;
}




void DistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const DistanceConstraint&>(other));
}




void DistanceConstraint::operator=(const DistanceConstraint& other)
{
    Distance::operator=(other);
    ConstrainedSketchEntity::operator=(other);
}







defineType(FixedDistanceConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, FixedDistanceConstraint, addParserRule);




FixedDistanceConstraint::FixedDistanceConstraint(
    VectorPtr p1, VectorPtr p2, VectorPtr planeNormal,
    const std::string& layerName
    )
    : DistanceConstraint(p1, p2, planeNormal, layerName)
{
    auto ps = defaultParameters();
    ps.extend(ParameterSet({
        {"distance", std::make_shared<DoubleParameter>(arma::norm(p2->value()-p1->value(), 2), "target value")}
    }));
    changeDefaultParameters(ps);
}




double FixedDistanceConstraint::targetValue() const
{
    return parameters().getDouble("distance");
}


void FixedDistanceConstraint::setTargetValue(double dist)
{
    parametersRef().setDouble("distance", dist);
}


void FixedDistanceConstraint::scaleSketch(double scaleFactor)
{
    auto& dp=parametersRef().get<DoubleParameter>("distance");
    dp.set(dp() * scaleFactor);
}




void FixedDistanceConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels)
            + ", "
            + pointSpec(p2_, script, entityLabels)
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void FixedDistanceConstraint::addParserRule(ConstrainedSketchGrammar &ruleset, MakeDefaultGeometryParametersFunction)
{
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ','
             > ruleset.r_point
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'
             )
                [ qi::_a = phx::bind(
                     &FixedDistanceConstraint::create<VectorPtr, VectorPtr, VectorPtr, const std::string&>,
                     qi::_2, qi::_3,
                     phx::bind(&ConstrainedSketch::sketchPlaneNormal, ruleset.sketch),
                     qi::_4 ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5, boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}




void FixedDistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const FixedDistanceConstraint&>(other));
}




void FixedDistanceConstraint::operator=(const FixedDistanceConstraint& other)
{
    DistanceConstraint::operator=(other);
}







defineType(LinkedDistanceConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, LinkedDistanceConstraint, addParserRule);




LinkedDistanceConstraint::LinkedDistanceConstraint(
    VectorPtr p1, VectorPtr p2,
    ScalarPtr dist,
    VectorPtr planeNormal,
    const std::string& layerName,
    const std::string& distExpr )

  : DistanceConstraint(p1, p2, planeNormal, layerName),
    distExpr_(distExpr), distance_(dist)
{}




double LinkedDistanceConstraint::targetValue() const
{
    return distance_->value();
}




void LinkedDistanceConstraint::scaleSketch(double scaleFactor)
{}




void LinkedDistanceConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{
    int myLabel=entityLabels.at(this);
    script.insertCommandFor(
        myLabel,
        type() + "( "
            + lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p1_, script, entityLabels) + ", "
            + pointSpec(p2_, script, entityLabels) + ", "
            + distExpr_
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




void LinkedDistanceConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
    MakeDefaultGeometryParametersFunction )
{
    if (ruleset.iscadScriptRules)
    {
        typedef
            boost::spirit::qi::rule<
                std::string::iterator,
                ConstrainedSketchGrammar::ParserRuleResult(),
                insight::cad::parser::skip_grammar,
                boost::spirit::qi::locals<
                    std::shared_ptr<ConstrainedSketchEntity>,
                    insight::cad::ScalarPtr >
            > ExtParserRule;

        namespace qi=boost::spirit::qi;
        namespace phx=boost::phoenix;

        auto &rule = ruleset.addAdditionalRule(
            std::make_shared<ExtParserRule>(
            ( '('
             > qi::int_ > ','
             > ruleset.r_point > ','
             > ruleset.r_point > ','
             > qi::as_string[
                 qi::raw[ruleset.iscadScriptRules->r_scalarExpression[phx::ref(qi::_b) = qi::_1]]
              ]
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'  )
                [ qi::_a = phx::bind(
                     &LinkedDistanceConstraint::create<
                         VectorPtr, VectorPtr,
                         ScalarPtr, VectorPtr,
                         const std::string&, const std::string&>,
                     qi::_2, qi::_3, qi::_b,
                     phx::bind(&ConstrainedSketch::sketchPlaneNormal, ruleset.sketch),
                     qi::_5, qi::_4 ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet,
                           qi::_a, qi::_6, boost::filesystem::path(".")),
                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                     qi::_1, qi::_a ) ]
                )
        );

        ruleset.entityRules.add( typeName, rule );
    }
}




void LinkedDistanceConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const LinkedDistanceConstraint&>(other));
}




void LinkedDistanceConstraint::operator=(const LinkedDistanceConstraint& other)
{
    DistanceConstraint::operator=(other);
    distance_=other.distance_;
}


}
}
