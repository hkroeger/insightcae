#include "iqvtkpointoncurveconstraint.h"


#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"


IQVTKPointOnCurveConstraint::IQVTKPointOnCurveConstraint(
    std::shared_ptr<insight::cad::SketchPoint const> p,
    std::shared_ptr<insight::cad::Feature const> curve )
    : p_(p),
    curve_(curve)
{}

std::vector<vtkSmartPointer<vtkProp> >
IQVTKPointOnCurveConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption("F");
    caption->SetAttachmentPoint(
        arma::mat(p_->value()).memptr()
        );
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);

    return {caption};
}

int IQVTKPointOnCurveConstraint::nConstraints() const
{
    return 1;
}

double IQVTKPointOnCurveConstraint::getConstraintError(unsigned int iConstraint) const
{
    insight::assertion(
        iConstraint==0,
        "invalid constraint index");

    return curve_->minDist(p_->value());
}

void IQVTKPointOnCurveConstraint::scaleSketch(double scaleFactor)
{
}

void IQVTKPointOnCurveConstraint::generateScriptCommand(insight::cad::ConstrainedSketchScriptBuffer &script, const std::map<const ConstrainedSketchEntity *, int> &entityLabels) const
{

}
