#include "singlesymbolconstraint.h"

#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"

namespace insight {
namespace cad {


std::vector<vtkSmartPointer<vtkProp> >
SingleSymbolConstraint::createActor() const
{
    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption(symbolText().c_str());
    caption->BorderOff();
    caption->SetAttachmentPoint( symbolLocation().memptr() );
    caption->GetTextActor()->SetTextScaleModeToNone(); //key: fix the font size
    caption->GetCaptionTextProperty()->SetColor(0,0,0);
    caption->GetCaptionTextProperty()->SetFontSize(10);
    caption->GetCaptionTextProperty()->SetFrame(false);
    caption->GetCaptionTextProperty()->SetShadow(false);
    caption->GetCaptionTextProperty()->FrameOff();
    caption->GetCaptionTextProperty()->ShadowOff();
    caption->GetCaptionTextProperty()->BoldOff();

    return {caption};
}

bool SingleSymbolConstraint::isInside(SelectionRect r) const
{
    return r.isInside(symbolLocation());
}


} // namespace cad
} // namespace insight
