#include "iqvtkcadmodel3dviewerpanning.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkCamera.h"




void IQVTKCADModel3DViewerPanning::ComputeWorldToDisplay(
        double x, double y, double z, double displayPt[3])
{
    viewer().renderer()->SetWorldPoint(x, y, z, 1.0);
    viewer().renderer()->WorldToDisplay();
    viewer().renderer()->GetDisplayPoint(displayPt);
}




void IQVTKCADModel3DViewerPanning::ComputeDisplayToWorld(
        double x, double y, double z, double worldPt[4])
{
    viewer().renderer()->SetDisplayPoint(x, y, z);
    viewer().renderer()->DisplayToWorld();
    viewer().renderer()->GetWorldPoint(worldPt);
    if (worldPt[3])
    {
        worldPt[0] /= worldPt[3];
        worldPt[1] /= worldPt[3];
        worldPt[2] /= worldPt[3];
        worldPt[3] = 1.0;
    }
}




void IQVTKCADModel3DViewerPanning::pan(int x, int y)
{
    if (hasLastMouseLocation())
    {

        double viewFocus[4], focalDepth, viewPoint[3];
        double newPickPoint[4], oldPickPoint[4], motionVector[3];

        // Calculate the focal depth since we'll be using it a lot

        vtkCamera* camera = viewer().renderer()->GetActiveCamera();
        camera->GetFocalPoint(viewFocus);
        this->ComputeWorldToDisplay(viewFocus[0], viewFocus[1], viewFocus[2], viewFocus);
        focalDepth = viewFocus[2];

        auto p1=viewer().widgetCoordsToVTK(QPoint(x,y));
        this->ComputeDisplayToWorld(
                    p1.x(), p1.y(), focalDepth, newPickPoint);

        // Has to recalc old mouse point since the viewport has moved,
        // so can't move it outside the loop

        auto p2=viewer().widgetCoordsToVTK(lastMouseLocation());
        this->ComputeDisplayToWorld(
                    p2.x(), p2.y(), focalDepth, oldPickPoint);

        // Camera motion is reversed

        motionVector[0] = oldPickPoint[0] - newPickPoint[0];
        motionVector[1] = oldPickPoint[1] - newPickPoint[1];
        motionVector[2] = oldPickPoint[2] - newPickPoint[2];

        camera->GetFocalPoint(viewFocus);
        camera->GetPosition(viewPoint);
        camera->SetFocalPoint(
                    motionVector[0] + viewFocus[0], motionVector[1] + viewFocus[1], motionVector[2] + viewFocus[2]);

        camera->SetPosition(
                    motionVector[0] + viewPoint[0], motionVector[1] + viewPoint[1], motionVector[2] + viewPoint[2]);

        vtkRenderWindowInteractor* rwi = viewer().interactor();
        if (rwi->GetLightFollowCamera())
        {
            viewer().renderer()->UpdateLightsGeometryToFollowCamera();
        }

        viewer().scheduleRedraw();
    }
}




IQVTKCADModel3DViewerPanning::IQVTKCADModel3DViewerPanning(
        IQVTKCADModel3DViewer &viewWidget, const QPoint point)
    : ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget, point)
{}

void IQVTKCADModel3DViewerPanning::start()
{}




bool IQVTKCADModel3DViewerPanning::onMouseMove
(
        Qt::MouseButtons buttons,
        const QPoint point,
        Qt::KeyboardModifiers curFlags
)
{
    pan(point.x(), point.y());
    return ViewWidgetAction<IQVTKCADModel3DViewer>::onMouseMove(buttons, point, curFlags);
}
