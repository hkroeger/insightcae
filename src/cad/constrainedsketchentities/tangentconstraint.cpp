#include "tangentconstraint.h"
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


defineType(TangentConstraint);


std::pair<TangentConstraint::Attachment,TangentConstraint::Attachment>
TangentConstraint::commonPoint() const
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


TangentConstraint::TangentConstraint(
    std::shared_ptr<SingleEdgeFeature> line1,
    std::shared_ptr<SingleEdgeFeature> line2,
    const std::string& layerName)
: SingleSymbolConstraint(layerName),
  line1_(line1), line2_(line2)
{}

std::string TangentConstraint::symbolText() const
{
    return "T";
}

arma::mat TangentConstraint::symbolLocation() const
{
    return (( commonPoint().first==Start) ?
                line1_->start()
              : line1_->end()
            )->value() ;
}


int TangentConstraint::nConstraints() const
{
    return 1;
}

double TangentConstraint::getConstraintError(unsigned int iConstraint) const
{
    arma::mat d1 = commonPoint().first == Start ?
                       line1_->startTangent()->value()
                     : line1_->endTangent()->value();

    arma::mat d2 = commonPoint().second == Start ?
                       line2_->startTangent()->value()
                     : line2_->endTangent()->value();

    double l1=arma::norm(d1, 2), l2=arma::norm(d2, 2);
    if ( (l1>SMALL) && (l2>SMALL) )
    {
        d1/=l1;
        d2/=l2;
    }

    //auto err = pow(arma::dot(d1, d2),2) - 1.; // bad convergence
    if (planeNormal_.n_elem==3)
    {
        // signed sine of the enclosed angle:
        // smooth through the solution (in contrast to its magnitude)
        return arma::dot(arma::cross(d1, d2), planeNormal_);
    }
    else
    {
        return arma::norm(arma::cross(d1, d2), 2);
    }
}

void TangentConstraint::prepareConstraintSolve(const arma::mat& sketchPlaneNormal)
{
    planeNormal_.reset();
    if (arma::norm(sketchPlaneNormal, 2) > SMALL)
    {
        planeNormal_ = arma::normalise(sketchPlaneNormal);
    }
}

void TangentConstraint::scaleSketch(double scaleFactor)
{}


void TangentConstraint::generateScriptCommand(
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


addToStaticFunctionTable(ConstrainedSketchEntity, TangentConstraint, addParserRule);



void TangentConstraint::addParserRule(
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
                     &TangentConstraint::create<
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
TangentConstraint::dependencies() const
{
    auto l1 = std::dynamic_pointer_cast<ConstrainedSketchEntity>(line1_);
    auto l2 = std::dynamic_pointer_cast<ConstrainedSketchEntity>(line2_);
    insight::assertion( l1 && l2, "both curves have to be constrained sketch entities");
    return { l1, l2 };
}


void TangentConstraint::replaceDependency(
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



void TangentConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const TangentConstraint&>(other));
}

void TangentConstraint::operator=(const TangentConstraint& other)
{
    line1_=other.line1_;
    line2_=other.line2_;
    ConstrainedSketchEntity::operator=(other);
}


ConstrainedSketchEntityPtr TangentConstraint::clone() const
{
    auto cl=TangentConstraint::create( line1_, line2_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef().assignFrom( parameters() );
    return cl;
}


}
}
