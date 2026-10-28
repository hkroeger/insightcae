#include "featureannotation.h"

#include "ivtkoccshape.h"
#include "parameterlisthash.h"

#include "TopExp_Explorer.hxx"

#include "vtkFeatureEdges.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkCaptionActor2D.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkProperty.h"


namespace insight {
namespace cad {




FeatureAnnotation::FeatureAnnotation(
    FeaturePtr feature,
    const arma::mat& attachmentPoint,
    const std::string& label,
    const arma::mat& color )
    : feature_(feature),
      attachmentPoint_(attachmentPoint),
      label_(label),
      color_(color)
{}




vtkSmartPointer<vtkPolyData> FeatureAnnotation::outline() const
{
    if (TopExp_Explorer(feature_->shape(), TopAbs_FACE).More())
    {
        // boundary edges of the surfaces
        auto fe = vtkSmartPointer<vtkFeatureEdges>::New();
        fe->SetInputData(feature_->triangulationToVTK());
        fe->BoundaryEdgesOn();
        fe->FeatureEdgesOff();
        fe->ManifoldEdgesOff();
        fe->NonManifoldEdgesOff();
        fe->Update();
        return fe->GetOutput();
    }
    else
    {
        // edges only
        auto edges = vtkSmartPointer<ivtkOCCShape>::New();
        edges->SetShape(feature_->shape());
        edges->SetRepresentation(insight::Wireframe);
        edges->Update();
        return edges->GetOutput();
    }
}




std::vector<vtkSmartPointer<vtkProp> > FeatureAnnotation::createVTKRepr() const
{
    std::vector<vtkSmartPointer<vtkProp> > result;

    // --- Outline (analogous to SilhouetteHighlighter) ---
    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(outline());

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
    actor->GetProperty()->SetColor(color_(0), color_(1), color_(2));
    actor->GetProperty()->SetLineWidth(3);
    actor->PickableOff();
    result.push_back(actor);

    // --- Caption with leader line (analogous to TextActorHighlighter) ---
    double ap[3] = { attachmentPoint_(0), attachmentPoint_(1), attachmentPoint_(2) };

    auto caption = vtkSmartPointer<vtkCaptionActor2D>::New();
    caption->SetCaption(label_.c_str());
    caption->BorderOff();
    caption->SetAttachmentPoint(ap);
    caption->GetTextActor()->SetTextScaleModeToNone();
    caption->GetCaptionTextProperty()->SetColor(color_(0), color_(1), color_(2));
    caption->GetCaptionTextProperty()->SetFontSize(13);
    caption->GetCaptionTextProperty()->FrameOff();
    caption->GetCaptionTextProperty()->ShadowOff();
    caption->GetCaptionTextProperty()->BoldOff();
    caption->PickableOff();
    result.push_back(caption);

    return result;
}




void FeatureAnnotation::write(std::ostream&) const
{}




size_t FeatureAnnotation::calcHash() const
{
    ParameterListHash h;
    h+=label_;
    h+=*feature_;
    for (int i=0; i<3; ++i)
    {
        h+=attachmentPoint_(i);
        h+=color_(i);
    }
    return h.getHash();
}




void FeatureAnnotation::build()
{}




}
}
