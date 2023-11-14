#include "iqvtkfixedpoint.h"


#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"

#include "base/parameters.h"

defineType(IQVTKFixedPoint);

IQVTKFixedPoint::IQVTKFixedPoint(
    insight::cad::SketchPointPtr p, const std::string& layerName )
    : IQVTKConstrainedSketchEntity(layerName),
      p_(p)
{
    auto c=p->coords2D();
    changeDefaultParameters(
        insight::ParameterSet({
           {"x", std::make_shared<insight::DoubleParameter>(c(0), "x location")},
           {"y", std::make_shared<insight::DoubleParameter>(c(1), "y location")}
        })
        );
}

std::vector<vtkSmartPointer<vtkProp> >
IQVTKFixedPoint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("X");
    caption->BorderOff();
    caption->SetAttachmentPoint(p_->value().memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->FrameOff();
    caption->GetCaptionTextProperty()->ShadowOff();
    caption->GetCaptionTextProperty()->BoldOff();

    return {caption};
}

int IQVTKFixedPoint::nConstraints() const
{
    return 2;
}

double IQVTKFixedPoint::getConstraintError(unsigned int iConstraint) const
{
    auto p = p_->coords2D();
    switch (iConstraint)
    {
    case 0:
        return p(0)-parameters().getDouble("x");
    case 1:
        return p(1)-parameters().getDouble("y");
    };

    throw insight::Exception(
        "invalid constraint id: %d", iConstraint
        );

    return std::nan("NAN");
}

void IQVTKFixedPoint::scaleSketch(double scaleFactor)
{
    auto& x = parametersRef().get<insight::DoubleParameter>("x");
    x.set(x() * scaleFactor);
    auto& y = parametersRef().get<insight::DoubleParameter>("y");
    y.set(y() * scaleFactor);
}

void IQVTKFixedPoint::generateScriptCommand(
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
addToStaticFunctionTable(ConstrainedSketchEntity, IQVTKFixedPoint, addParserRule);
}}

void IQVTKFixedPoint::addParserRule(
    insight::cad::ConstrainedSketchGrammar &ruleset,
    insight::cad::MakeDefaultGeometryParametersFunction )
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
                 &IQVTKFixedPoint::create<insight::cad::SketchPointPtr, const std::string&>,
                 phx::bind(&ConstrainedSketch::get<SketchPoint>, ruleset.sketch, qi::_2), qi::_3
                 ),
                phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_a, qi::_4, "."),
                qi::_val = phx::construct<ConstrainedSketchGrammar::ParserRuleResult>(qi::_1, qi::_a) ]
            );
}

std::set<std::comparable_weak_ptr<insight::cad::ConstrainedSketchEntity> > IQVTKFixedPoint::dependencies() const
{
    return { p_ };
}

void IQVTKFixedPoint::replaceDependency(
    const std::weak_ptr<ConstrainedSketchEntity> &entity,
    const std::shared_ptr<ConstrainedSketchEntity> &newEntity )
{
    if (auto p = std::dynamic_pointer_cast<insight::cad::SketchPoint>(newEntity))
    {
        if (std::dynamic_pointer_cast<ConstrainedSketchEntity>(p_) == entity)
        {
            p_ = p;
        }
    }
}


void IQVTKFixedPoint::operator=(const ConstrainedSketchEntity& other)
{
    operator=(dynamic_cast<const IQVTKFixedPoint&>(other));
}

void IQVTKFixedPoint::operator=(const IQVTKFixedPoint& other)
{
    p_=other.p_;
    IQVTKConstrainedSketchEntity::operator=(other);
}
