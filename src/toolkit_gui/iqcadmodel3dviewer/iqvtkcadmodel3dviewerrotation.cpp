#include "iqvtkcadmodel3dviewerrotation.h"

#include "iqvtkcadmodel3dviewer.h"

#include "vtkCamera.h"

void IQVTKCADModel3DViewerRotation::rotate(int x, int y)
{

    if (hasLastMouseLocation())
    {

        int dx = x - lastMouseLocation().x();
        int dy = lastMouseLocation().y() - y;

        int* size = viewer().renWin()->GetSize();

        double delta_elevation = -20.0 / size[1];
        double delta_azimuth = -20.0 / size[0];

        double rxf = dx * delta_azimuth * this->MotionFactor;
        double ryf = dy * delta_elevation * this->MotionFactor;

        vtkCamera* camera = viewer().renderer()->GetActiveCamera();
        camera->Azimuth(rxf);
        camera->Elevation(ryf);
        camera->OrthogonalizeViewUp();

        if (this->AutoAdjustCameraClippingRange)
        {
          viewer().renderer()->ResetCameraClippingRange();
        }

        vtkRenderWindowInteractor* rwi = viewer().interactor();
        if (rwi->GetLightFollowCamera())
        {
          viewer().renderer()->UpdateLightsGeometryToFollowCamera();
        }

        viewer().scheduleRedraw();
    }
}




IQVTKCADModel3DViewerRotation::IQVTKCADModel3DViewerRotation(
        IQVTKCADModel3DViewer &viewWidget,
        const QPoint point)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget, point, true)
{}

void IQVTKCADModel3DViewerRotation::start()
{}




bool IQVTKCADModel3DViewerRotation::onMouseMove
(
 Qt::MouseButtons buttons,
 const QPoint point,
 Qt::KeyboardModifiers curFlags
 )
{
    rotate( point.x(), point.y() );
    return ViewWidgetAction<IQVTKCADModel3DViewer>::onMouseMove(buttons, point, curFlags);
}
