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






defineType(DistanceConstraint);
addToStaticFunctionTable(ConstrainedSketchEntity, DistanceConstraint, addParserRule);

size_t DistanceConstraint::calcHash() const
{
    ParameterListHash h;
    h+=p1_->value();
    h+=p2_->value();
    h+=targetValue();
    return h.getHash();
}


DistanceConstraint::DistanceConstraint(VectorPtr p1, VectorPtr p2, double targetValue)
    : Distance(p1, p2)
{
    changeDefaultParameters(
                ParameterSet({
                                 {"distance", new DoubleParameter(targetValue, "target value")}
                             })
                );
}

double DistanceConstraint::targetValue() const
{
    return parameters().getDouble("distance");
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
    return (distance_ - targetValue()) /*/ stabilize(targetValue(), SMALL)*/;
}

void DistanceConstraint::scaleSketch(double scaleFactor)
{
    parametersRef().get<DoubleParameter>("distance")() *= scaleFactor;
}

void DistanceConstraint::generateScriptCommand(
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
            + parameterString()
            + ")"
        );
}

void DistanceConstraint::addParserRule(ConstrainedSketchGrammar &ruleset, MakeDefaultGeometryParametersFunction)
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
             > ruleset.r_parameters >
             ')'
             )
                [ qi::_val = phx::bind(
                     &DistanceConstraint::create<VectorPtr, VectorPtr, double>, qi::_2, qi::_3, 1.0),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_4, "."),
                 phx::insert(
                     phx::ref(ruleset.labeledEntities),
                     phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
            );
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

}
}
