#include "iqvtkkeepfixedsizecallback.h"

#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkTransform.h"

#include "base/exception.h"
#include "iqcadmodel3dviewer.h"

IQVTKKeepFixedSize::IQVTKKeepFixedSize()
    : trsf_(decltype(trsf_)::New())
{
    auto t = vtkSmartPointer<vtkTransform>::New();
    trsf_->SetTransform(t);
}


IQVTKKeepFixedSize *IQVTKKeepFixedSize::New()
{
    return new IQVTKKeepFixedSize();
}


void IQVTKKeepFixedSize::SetViewer(const IQCADModel3DViewer *viewer)
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

vtkAlgorithmOutput *IQVTKKeepFixedSize::GetOutputPort()
{
   return trsf_->GetOutputPort();
}

void IQVTKKeepFixedSize::Execute(vtkObject *caller, unsigned long eventId, void *callData)
{
   insight::assertion(viewer_, "viewer must be set!");

   double newScale = initialScale_ / viewer_->getScale();

   auto t = vtkTransform::SafeDownCast(trsf_->GetTransform());
   t->Identity();
   t->Scale(newScale, newScale, newScale);
}
