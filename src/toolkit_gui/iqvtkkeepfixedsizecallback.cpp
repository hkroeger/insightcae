#include "iqvtkkeepfixedsizecallback.h"

#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkTransform.h"

#include "base/exception.h"
#include "base/spatialtransformation.h"
#include "iqvtkcadmodel3dviewer.h"


using namespace insight;

IQVTKKeepFixedSize::IQVTKKeepFixedSize()
  : trsf_(decltype(trsf_)::New()),
    pref_(vec3Zero())
{
    auto t = vtkSmartPointer<vtkTransform>::New();
    trsf_->SetTransform(t);
}


IQVTKKeepFixedSize *IQVTKKeepFixedSize::New()
{
    return new IQVTKKeepFixedSize();
}


void IQVTKKeepFixedSize::SetViewer(const IQVTKCADModel3DViewer *viewer)
{
    viewer_=viewer;
    initialScale_=viewer_->getScale();
}

void IQVTKKeepFixedSize::SetInputData(vtkPolyData *inputData)
{
    trsf_->SetInputData(inputData);
}

void IQVTKKeepFixedSize::SetInputConnection(vtkAlgorithmOutput *inputPort)
{
    trsf_->SetInputConnection(inputPort);
}

void IQVTKKeepFixedSize::SetPRef(const arma::mat &pref)
{
    pref_=pref;
}

vtkAlgorithmOutput *IQVTKKeepFixedSize::GetOutputPort()
{
   return trsf_->GetOutputPort();
}

void IQVTKKeepFixedSize::Execute(vtkObject *caller, unsigned long eventId, void *callData)
{
   if (eventId == vtkCommand::EventIds::ModifiedEvent)
   {
       insight::assertion(viewer_, "viewer must be set!");

       auto& b = viewer_->sceneBounds();
       double
           dx=std::fabs(b.xmax-b.xmin),
           dy=std::fabs(b.ymax-b.ymin),
           dz=std::fabs(b.zmax-b.zmin);

       initialScale_ = std::max(std::max(dx, dy), dz);

       double newScale = initialScale_/* / viewer_->getScale()*/;

       SpatialTransformation st(-pref_);
       st.appendTransformation(SpatialTransformation(newScale));
       st.appendTransformation(SpatialTransformation(pref_));

       trsf_->SetTransform(st.toVTKTransform());

//       auto t = vtkTransform::SafeDownCast(trsf_->GetTransform());
//       t->Identity();
//       t->Scale(newScale, newScale, newScale);
   }
}
