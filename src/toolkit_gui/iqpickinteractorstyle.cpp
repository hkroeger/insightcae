#include "iqpickinteractorstyle.h"

#include "vtkObjectFactory.h"
#include "vtkNew.h"
#include "vtkPropPicker.h"
#include "vtkProp.h"
#include "vtkProp3D.h"
#include "vtkRenderWindowInteractor.h"



vtkStandardNewMacro(IQPickInteractorStyle);




IQPickInteractorStyle::IQPickInteractorStyle()
    : vtkInteractorStyleTrackballCamera()
{
    lastClickPos[0]=lastClickPos[1]=0;
}




void IQPickInteractorStyle::OnLeftButtonDown()
{
    auto* rwi = this->Interactor;

    rwi->GetEventPosition(
              lastClickPos[0], lastClickPos[1] );

    vtkInteractorStyleTrackballCamera::OnLeftButtonDown();
}




void IQPickInteractorStyle::OnLeftButtonUp()
{
  auto* rwi = this->Interactor;

  int clickPos[2];
  rwi->GetEventPosition(clickPos[0], clickPos[1]);

  if (clickPos[0]==lastClickPos[0] && clickPos[1]==lastClickPos[1])
  {
      // Pick from this location.
      vtkNew<vtkPropPicker> picker;
      picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

      double* pos = picker->GetPickPosition();
      std::cout
              << "Pick position (world coordinates) is: "
              << pos[0] << " " << pos[1] << " " << pos[2]
              << std::endl;

      auto pickedActor = picker->GetActor();

      if (pickedActor == nullptr)
      {
        std::cout << "No actor picked." << std::endl;

        HighlightProp(nullptr);
      }
      else
      {
        auto pac=picker->GetActor();
        std::cout << "Picked actor: " << pac << std::endl;

        HighlightProp( picker->GetProp3D() );

        FindPokedRenderer(clickPos[0], clickPos[1]);
        rwi->StartPickCallback();

        if (auto* picker = rwi->GetPicker())
        {
          picker->Pick(clickPos[0], clickPos[1], 0.0, this->CurrentRenderer);
        }
        rwi->EndPickCallback();
      }
  }

  vtkInteractorStyleTrackballCamera::OnLeftButtonUp();
}




void IQPickInteractorStyle::OnRightButtonDown()
{

  this->GetInteractor()->GetEventPosition(
              lastClickPos[0], lastClickPos[1] );

  vtkInteractorStyleTrackballCamera::OnRightButtonDown();
}




void IQPickInteractorStyle::OnRightButtonUp()
{

  int clickPos[2];
  this->GetInteractor()->GetEventPosition(clickPos[0], clickPos[1]);

  if (clickPos[0]==lastClickPos[0] && clickPos[1]==lastClickPos[1])
  {
      // Pick from this location.
      vtkNew<vtkPropPicker> picker;
      picker->Pick(clickPos[0], clickPos[1], 0, this->GetDefaultRenderer());

      double* pos = picker->GetPickPosition();
      std::cout
              << "Pick position (world coordinates) is: "
              << pos[0] << " " << pos[1] << " " << pos[2]
              << std::endl;

      auto pickedActor = picker->GetActor();

      if (pickedActor == nullptr)
      {
        std::cout << "No actor picked." << std::endl;

        HighlightProp(nullptr);
      }
      else
      {
        auto pac=picker->GetActor();
        std::cout << "Picked actor RM: " << pac << std::endl;

        Q_EMIT contextMenuRequested(pac);

        HighlightProp( picker->GetProp3D() );
      }
  }

  vtkInteractorStyleTrackballCamera::OnRightButtonUp();
}

