#include "iqvtkfixedpoint.h"


#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

#include "base/parameters.h"

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
    return 1;
}

double IQVTKFixedPoint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint id: %d", iConstraint );
    auto p = p_->coords2D();
    double x=parameters().getDouble("x");
    double y=parameters().getDouble("y");
    return pow(p(0)-x,2), pow(p(1)-y, 2);
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

}
