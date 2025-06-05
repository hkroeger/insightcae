#include "horizontalconstraint.h"

#include "constrainedsketch.h"
#include "constrainedsketchgrammar.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"


namespace insight
{
namespace cad
{


defineType(HorizontalConstraint);




HorizontalConstraint::HorizontalConstraint(
    std::shared_ptr<insight::cad::Line> line,
    const std::string& layerName )
    : SingleSymbolConstraint(layerName), line_(line)
{}


std::string HorizontalConstraint::symbolText() const
{
    return "H";
}

arma::mat HorizontalConstraint::symbolLocation() const
{
    return 0.5*
           ( line_->start()->value()
                  +
             line_->end()->value() );
}




int HorizontalConstraint::nConstraints() const
{
    return 1;
}




double HorizontalConstraint::getConstraintError(
    unsigned int iConstraint) const
{
    auto p0 = std::dynamic_pointer_cast<SketchPoint>(line_->start());
    auto p1 = std::dynamic_pointer_cast<SketchPoint>(line_->end());
    insight::assertion(bool(p0), "only lines with sketch end points are allowed!");
    insight::assertion(bool(p1), "only lines with sketch end points are allowed!");
    arma::mat p02 = p0->coords2D();
    arma::mat p12 = p1->coords2D();
    arma::mat d2 = p12-p02;
    return d2(1);
}




void HorizontalConstraint::scaleSketch(double scaleFactor)
{}




void HorizontalConstraint::generateScriptCommand(
    insight::cad::ConstrainedSketchScriptBuffer &script,
    const std::map<const ConstrainedSketchEntity *, int> &entityLabels
    ) const
{
    int myLabel=entityLabels.at(this);

    line_->generateScriptCommand(script, entityLabels);

    script.insertCommandFor(
        myLabel,
        type() + "( "
            + boost::lexical_cast<std::string>(myLabel) + ", "
            + boost::lexical_cast<std::string>(entityLabels.at(line_.get()))
            + ", layer " + layerName()
            + parameterString()
            + ")"
        );
}




namespace insight { namespace cad {
addToStaticFunctionTable(ConstrainedSketchEntity, HorizontalConstraint, addParserRule);
}}




void HorizontalConstraint::addParserRule(
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
             ')'
         )
            [ qi::_a = phx::bind(
                 &HorizontalConstraint::create<std::shared_ptr<Line>, const std::string& >,
                 phx::bind(&ConstrainedSketch::get<Line>, ruleset.sketch, qi::_2), qi::_3 ),
              phx::bind(&ConstrainedSketchParametersDelegate::changeDefaultParameters, &pd, *qi::_a),
              phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4, "."),
              qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}




std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> >
HorizontalConstraint::dependencies() const
{
    return { line_ };
}




void HorizontalConstraint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto l = std::dynamic_pointer_cast<Line>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(line_) == entity)
        {
            line_ = l;
        }
    }
}




void HorizontalConstraint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const HorizontalConstraint&>(other));
}




ConstrainedSketchEntityPtr HorizontalConstraint::clone() const
{
    auto cl=HorizontalConstraint::create( line_, layerName() );

    cl->changeDefaultParameters(defaultParameters());
    cl->parametersRef() = parameters();
    return cl;
}




void HorizontalConstraint::operator=(const HorizontalConstraint& other)
{
    line_=other.line_;
    ConstrainedSketchEntity::operator=(other);
}

}
}
