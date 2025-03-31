#include "fixedpointconstraint.h"


#include "base/parameters/subsetparameter.h"
#include "base/parameterset.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include "base/parameters.h"

namespace insight
{
namespace cad
{

defineType(FixedPointConstraint);

FixedPointConstraint::FixedPointConstraint(
    insight::cad::SketchPointPtr p, const std::string& layerName )
    : SingleSymbolConstraint(layerName),
      p_(p)
{
    auto c=p->coords2D();

    insight::ParameterSet::Entries ps;
    ps.emplace("x", std::make_unique<insight::DoubleParameter>(c(0), "x location"));
    ps.emplace("y", std::make_unique<insight::DoubleParameter>(c(1), "y location"));

    changeDefaultParameters( *insight::ParameterSet::create(std::move(ps), "") );
}


std::string FixedPointConstraint::symbolText() const
{
    return "F";
}


arma::mat FixedPointConstraint::symbolLocation() const
{
    return p_->value();
}


int FixedPointConstraint::nConstraints() const
{
    return 2;
}

double FixedPointConstraint::getConstraintError(unsigned int iConstraint) const
{
    auto p = p_->coords2D();
    switch (iConstraint)
    {
    case 0:
        return p(0) - parameters().getDouble("x");
    case 1:
        return p(1) - parameters().getDouble("y");
    };

    throw insight::Exception(
        "invalid constraint id: %d", iConstraint
        );

    return std::nan("NAN");
}

void FixedPointConstraint::scaleSketch(double scaleFactor)
{
    auto& x = parametersRef().get<insight::DoubleParameter>("x");
    x.set(x() * scaleFactor);
    auto& y = parametersRef().get<insight::DoubleParameter>("y");
    y.set(y() * scaleFactor);
}

void FixedPointConstraint::generateScriptCommand(
    insight::cad::ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels
    ) const
{
    int myLabel=entityLabels.at(this);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + pointSpec(p_, script, entityLabels)
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}

namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, FixedPointConstraint, addParserRule);
}}

void FixedPointConstraint::addParserRule(
    ConstrainedSketchGrammar &ruleset,
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
             > qi::int_
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string()))
             > ruleset.r_parameters >
             ')' )
            [
                qi::_a = phx::bind(
                 &FixedPointConstraint::create<SketchPointPtr, const std::string&>,
                 phx::bind(&ConstrainedSketch::get<SketchPoint>, ruleset.sketch, qi::_2), qi::_3
                 ),
                phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4, "."),
                qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}

std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
FixedPointConstraint::dependencies() const
{
    return { p_ };
}

void FixedPointConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto p = std::dynamic_pointer_cast<SketchPoint>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p_) == entity)
        {
            p_ = p;
        }
    }
}


void FixedPointConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const FixedPointConstraint&>(other));
}

ConstrainedSketchEntityPtr FixedPointConstraint::clone() const
{
    auto cl=FixedPointConstraint::create( p_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}

void FixedPointConstraint::operator=(const FixedPointConstraint& other)
{
    p_=other.p_;
    ConstrainedSketchEntity::operator=(other);
}

}
}
