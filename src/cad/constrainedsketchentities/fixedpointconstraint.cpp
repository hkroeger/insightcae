#include "fixedpointconstraint.h"
#include "cadfeature.h"
#include "datum.h"

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
{}


std::string FixedPointConstraint::symbolText() const
{
    return "F";
}


arma::mat FixedPointConstraint::symbolLocation() const
{
    return p_->value();
}

std::pair<double,double> FixedPointConstraint::getXY() const
{
    return {
        parameters().getDouble("x"),
        parameters().getDouble("y")
    };
}

void FixedPointConstraint::setXY(const std::pair<double,double>& xy)
{
    parametersRef().setDouble("x", xy.first);
    parametersRef().setDouble("y", xy.second);
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
            + toString(myLabel) + ", "
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
             > qi::int_ > ',' //1
             > qi::int_ // 2
             > (( ',' >> qi::lit("layer") >> ruleset.r_label) | qi::attr(std::string())) // 3
             > ruleset.r_parameters > // 4
             ')' )
            [
                qi::_a = phx::bind(
                 &FixedPointConstraint::create<SketchPointPtr, const std::string&>,
                 phx::bind(&ConstrainedSketch::get<SketchPoint>, ruleset.sketch, qi::_2), qi::_3
                 ),
                phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
                phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4,
                    boost::filesystem::path(".")),
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

void FixedPointConstraint::ensureRequiredParameters()
{
    SingleSymbolConstraint::ensureRequiredParameters();

    // ensure that there are our required parameters
    auto c=p_->coords2D();
    parametersRef().getOrInsert<DoubleParameter>("x", c(0), "x location");
    parametersRef().getOrInsert<DoubleParameter>("y", c(1), "y location");
}


void FixedPointConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const FixedPointConstraint&>(other));
}

ConstrainedSketchEntityPtr FixedPointConstraint::clone() const
{
    auto cl=FixedPointConstraint::create( p_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef().assignFrom( parameters() );
    return cl;
}

void FixedPointConstraint::operator=(const FixedPointConstraint& other)
{
    p_=other.p_;
    ConstrainedSketchEntity::operator=(other);
}

}
}
