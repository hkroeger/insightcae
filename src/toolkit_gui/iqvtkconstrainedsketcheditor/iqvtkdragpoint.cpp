#include "iqvtkdragpoint.h"


#include "datum.h"


IQVTKDragPoint::IQVTKDragPoint(
    IQVTKConstrainedSketchEditor &editor,
    std::shared_ptr<insight::cad::SketchPoint> p )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(editor.viewer(), true),
    editor_(editor), p_(p)
{}




void IQVTKDragPoint::start()
{}




bool IQVTKDragPoint::onMouseDrag  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    EventType eventType )
{
    arma::mat pip=viewer().pointInPlane3D(
        editor_->plane()->plane(), point );

    arma::mat p2=viewer().pointInPlane2D(
        editor_->plane()->plane(), pip );

    p_->setCoords2D(p2(0), p2(1));

    editor_->invalidate();

    editor_->geometryChanged(
        editor_->findGeometry(p_)->first);

    if (eventType==Final) finishAction();

    return true;
}

