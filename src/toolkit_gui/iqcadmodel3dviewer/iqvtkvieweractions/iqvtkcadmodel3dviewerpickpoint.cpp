#include "iqvtkcadmodel3dviewerpickpoint.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkSmartPointer.h"
#include "vtkWorldPointPicker.h"



IQVTKCADModel3DViewerPickPoint::IQVTKCADModel3DViewerPickPoint(
    ViewWidgetActionHost<IQVTKCADModel3DViewer> &parent )
: ViewWidgetAction<IQVTKCADModel3DViewer>(parent, true)
{}



void IQVTKCADModel3DViewerPickPoint::start()
{}




bool IQVTKCADModel3DViewerPickPoint::onLeftButtonDown(Qt::KeyboardModifiers nFlags, const QPoint screenPos, bool afterDoubleClick)
{
    auto picker = vtkSmartPointer<vtkWorldPointPicker>::New();

    auto p = viewer().widgetCoordsToVTK(screenPos);
    picker->Pick(p.x(), p.y(), 0, viewer().renderer());

    arma::mat pt = insight::vec3Zero();
    picker->GetPickPosition(pt.memptr());
    Q_EMIT pickedPoint(pt);
    
    finishAction();

    return true;
}
