#include "iqvtkdragdimensionlineaction.h"

#include "datum.h"


IQVTKDragDimensionlineAction::IQVTKDragDimensionlineAction(
    IQVTKConstrainedSketchEditor &editor,
    std::shared_ptr<insight::cad::DistanceConstraint> dc )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(editor.viewer(), true),
    editor_(editor), dc_(dc)
{
}



void IQVTKDragDimensionlineAction::start()
{}




bool IQVTKDragDimensionlineAction::onMouseDrag(
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    EventType eventType )
{
    arma::mat pip=viewer().pointInPlane3D(
        editor_->plane()->plane(), point );

    dc_->setDimLineOffset(pip);

    editor_->invalidate();

    editor_->geometryChanged(
        editor_->findGeometry(dc_)->first);

    if (eventType==Final) finishAction();

    return true;
}




// bool IQVTKDragDimensionlineAction::onLeftButtonUp(
//     Qt::KeyboardModifiers nFlags,
//     const QPoint point,
//     bool lastClickWasDoubleClick )
// {
//     finishAction();
//     return true;
// }
