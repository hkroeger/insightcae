#include "iqvtkfixedpoint.h"


#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include "base/parameters.h"

defineType(IQVTKFixedPoint);

IQVTKFixedPoint::IQVTKFixedPoint(
    insight::cad::SketchPointPtr p  )
    : p_(p)
{
    auto c=p->coords2D();
    changeDefaultParameters(
        insight::ParameterSet({
            {"x", new insight::DoubleParameter(c(0), "x location")},
            {"y", new insight::DoubleParameter(c(1), "y location")}
        })
        );
}

std::vector<vtkSmartPointer<vtkProp> >
IQVTKFixedPoint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("X");
    caption->SetAttachmentPoint(p_->value().memptr());
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

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
        return parameters().getDouble("x");
    case 1:
        return parameters().getDouble("y");
    };

    throw insight::Exception(
        "invalid constraint id: %d", iConstraint
        );
}

void IQVTKFixedPoint::scaleSketch(double scaleFactor)
{
    parametersRef().get<insight::DoubleParameter>("x")() *= scaleFactor;
    parametersRef().get<insight::DoubleParameter>("y")() *= scaleFactor;
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
             > ruleset.r_parameters >
             ')' )
                [
                 qi::_val = phx::bind(
                     &IQVTKFixedPoint::create<insight::cad::SketchPointPtr>,
                     phx::bind(&insight::cad::ConstrainedSketchGrammar::lookupEntity<insight::cad::SketchPoint>, phx::ref(ruleset), qi::_2) ),
                 phx::bind(&ConstrainedSketchEntity::parseParameterSet, qi::_val, qi::_3, "."),
                 phx::insert(
                     phx::ref(ruleset.labeledEntities),
                     phx::construct<ConstrainedSketchGrammar::LabeledEntitiesMap::value_type>(qi::_1, qi::_val)) ]
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
