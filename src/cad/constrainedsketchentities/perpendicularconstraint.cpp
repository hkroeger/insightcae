#include "perpendicularconstraint.h"
#include "cadfeature.h"
#include "datum.h"
#include "base/exception.h"
#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"
#include <memory>


namespace insight
{
namespace cad
{


defineType(PerpendicularConstraint);


std::pair<PerpendicularConstraint::Attachment,PerpendicularConstraint::Attachment>
PerpendicularConstraint::commonPoint() const
{
    auto cp = line1_->start();
    if (cp==line2_->start()) return {Start, Start};
    if (cp==line2_->end()) return {Start, End};
    cp = line1_->end();
    if (cp==line2_->start()) return {End, Start};
    if (cp==line2_->end()) return {End, End};
    throw insight::Exception("lines do not have a common point!");
    return {Start,Start};
}


PerpendicularConstraint::PerpendicularConstraint(
    std::shared_ptr<SingleEdgeFeature> line1,
    std::shared_ptr<SingleEdgeFeature> line2,
    const std::string& layerName)
: SingleSymbolConstraint(layerName),
  line1_(line1), line2_(line2)
{}

std::string PerpendicularConstraint::symbolText() const
{
    return "P";
}

arma::mat PerpendicularConstraint::symbolLocation() const
{
    return (( commonPoint().first==Start) ?
                line1_->start()
              : line1_->end()
            )->value() ;
}


int PerpendicularConstraint::nConstraints() const
{
    return 1;
}

double PerpendicularConstraint::getConstraintError(unsigned int iConstraint) const
{
    arma::mat d1 = commonPoint().first == Start ?
                       line1_->startTangent()->value()
                     : line1_->endTangent()->value();

    arma::mat d2 = commonPoint().second == Start ?
                       line2_->startTangent()->value()
                     : line2_->endTangent()->value();

    return arma::dot(
        arma::normalise(d1),
        arma::normalise(d2) );
}

void PerpendicularConstraint::scaleSketch(double scaleFactor)
{}


void PerpendicularConstraint::generateScriptCommand(
    ConstrainedSketchScriptBuffer& script,
    const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const
{
    int myLabel=entityLabels.at(this);

    auto l1=std::dynamic_pointer_cast<ConstrainedSketchEntity>(line1_);
    auto l2=std::dynamic_pointer_cast<ConstrainedSketchEntity>(line2_);

    l1->generateScriptCommand(script, entityLabels);
    l2->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + toString(myLabel) + ", "
            + toString(entityLabels.at(l1.get())) + ", "
            + toString(entityLabels.at(l2.get()))
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}


addToStaticFunctionTable(ConstrainedSketchEntity, PerpendicularConstraint, addParserRule);



void PerpendicularConstraint::addParserRule(
    ConstrainedSketchGrammar& ruleset,
    const ConstrainedSketchParametersDelegate& pd )
{
    using namespace insight::cad;
    namespace qi=boost::spirit::qi;
    namespace phx=boost::phoenix;
    ruleset.entityRules.add
        (
            typeName,
            ( '('
             > qi::int_ > ','
             > qi::int_ > ','
             > qi::int_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')'
             )
                [
                 qi::_a = phx::bind(
                     &PerpendicularConstraint::create<
                        std::shared_ptr<SingleEdgeFeature>,
                        std::shared_ptr<SingleEdgeFeature>,
                        const std::string& >,
                     phx::bind(&ConstrainedSketch::get<SingleEdgeFeature>, ruleset.sketch, qi::_2),
                     phx::bind(&ConstrainedSketch::get<SingleEdgeFeature>, ruleset.sketch, qi::_3),
                    qi::_4 ),
                 phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_5,
                    boost::filesystem::path(".")),

                 qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(
                        qi::_1, qi::_a)
                ]
            );
}


std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
PerpendicularConstraint::dependencies() const
{
    auto l1 = std::dynamic_pointer_cast<ConstrainedSketchEntity>(line1_);
    auto l2 = std::dynamic_pointer_cast<ConstrainedSketchEntity>(line2_);
    insight::assertion( l1 && l2, "both curves have to be constrained sketch entities");
    return { l1, l2 };
}


void PerpendicularConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity>& entity,
    const std::shared_ptr<ConstrainedSketchEntity>& newEntity)
{
    if (auto l = std::dynamic_pointer_cast<SingleEdgeFeature>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line1_) == entity)
        {
            line1_ = l;
        }
        else if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line2_) == entity)
        {
            line2_ = l;
        }
    }
}



void PerpendicularConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const PerpendicularConstraint&>(other));
}

void PerpendicularConstraint::operator=(const PerpendicularConstraint& other)
{
    line1_=other.line1_;
    line2_=other.line2_;
    ConstrainedSketchEntity::operator=(other);
}


ConstrainedSketchEntityPtr PerpendicularConstraint::clone() const
{
    auto cl=PerpendicularConstraint::create( line1_, line2_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef().assignFrom( parameters() );
    return cl;
}


}
}
