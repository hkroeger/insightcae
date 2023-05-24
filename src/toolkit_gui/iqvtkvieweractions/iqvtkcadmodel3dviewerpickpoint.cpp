#include "iqvtkcadmodel3dviewerpickpoint.h"
#include "iqvtkcadmodel3dviewer.h"

#include "vtkSmartPointer.h"
#include "vtkWorldPointPicker.h"



IQVTKCADModel3DViewerPickPoint::IQVTKCADModel3DViewerPickPoint(
    IQVTKCADModel3DViewer &viewWidget )
: ViewWidgetAction<IQVTKCADModel3DViewer>(viewWidget)
{}




IQVTKCADModel3DViewerPickPoint::~IQVTKCADModel3DViewerPickPoint()
{}




bool IQVTKCADModel3DViewerPickPoint::onLeftButtonDown(Qt::KeyboardModifiers nFlags, const QPoint screenPos)
{
    auto picker = vtkSmartPointer<vtkWorldPointPicker>::New();

    auto p = viewer().widgetCoordsToVTK(screenPos);
    picker->Pick(p.x(), p.y(), 0, viewer().renderer());

    arma::mat pt = insight::vec3Zero();
    picker->GetPickPosition(pt.memptr());
    Q_EMIT pickedPoint(pt);

    setFinished();

    return true;
}
