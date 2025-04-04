#include "iqvtkcadmodel3dviewerpickpoint.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkSmartPointer.h"
#include "vtkWorldPointPicker.h"
#include <qdebug.h>
#include <qnamespace.h>



IQVTKCADModel3DViewerPickPoint::IQVTKCADModel3DViewerPickPoint(
    ViewWidgetActionHost<IQVTKCADModel3DViewer> &parent )
: ViewWidgetAction<IQVTKCADModel3DViewer>(parent, true)
{}



void IQVTKCADModel3DViewerPickPoint::start()
{}




bool IQVTKCADModel3DViewerPickPoint::onMouseClick  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point )
{
    if (btn&Qt::LeftButton)
    {
        auto picker = vtkSmartPointer<vtkWorldPointPicker>::New();

        auto p = viewer().widgetCoordsToVTK(point);
        picker->Pick(p.x(), p.y(), 0, viewer().renderer());

        arma::mat pt = insight::vec3Zero();
        picker->GetPickPosition(pt.memptr());
        Q_EMIT pickedPoint(pt);

        finishAction();

        return true;
    }

    return false;
}
