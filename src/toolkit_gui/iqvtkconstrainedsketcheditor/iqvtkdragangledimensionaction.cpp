#include "iqvtkdragangledimensionaction.h"


#include "datum.h"


IQVTKDragAngleDimensionAction::IQVTKDragAngleDimensionAction(
    IQVTKConstrainedSketchEditor &editor,
    std::shared_ptr<insight::cad::AngleConstraint> ac )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(editor.viewer(), true),
    editor_(editor), ac_(ac)
{}




void IQVTKDragAngleDimensionAction::start()
{}




bool IQVTKDragAngleDimensionAction::onMouseDrag  (
    Qt::MouseButtons btn,
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    EventType eventType )
{
    arma::mat pip=viewer().pointInPlane3D(
        editor_->plane()->plane(), point );

    ac_->setDimLineRadius( arma::norm(pip - ac_->centerPoint()->value(), 2) );

    editor_->invalidate();

    editor_->geometryChanged(
        editor_->findGeometry(ac_)->first);

    if (eventType==Final) finishAction();

    return true;
}




// bool IQVTKDragAngleDimensionAction::onLeftButtonUp(
//     Qt::KeyboardModifiers nFlags,
//     const QPoint point,
//     bool lastClickWasDoubleClick )
// {
//     finishAction();
//     return true;
// }
