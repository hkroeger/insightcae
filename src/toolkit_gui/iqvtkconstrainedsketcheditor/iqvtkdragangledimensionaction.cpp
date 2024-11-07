#include "iqvtkdragangledimensionaction.h"


#include "datum.h"


IQVTKDragAngleDimensionAction::IQVTKDragAngleDimensionAction(
    IQVTKConstrainedSketchEditor &editor,
    std::shared_ptr<insight::cad::AngleConstraint> ac )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(editor.viewer()),
    editor_(editor), ac_(ac)
{
}


IQVTKDragAngleDimensionAction::~IQVTKDragAngleDimensionAction()
{}



void IQVTKDragAngleDimensionAction::start()
{}




bool IQVTKDragAngleDimensionAction::onMouseMove
    (
        Qt::MouseButtons buttons,
        const QPoint point,
        Qt::KeyboardModifiers curFlags
        )
{
    arma::mat pip=viewer().pointInPlane3D(
        editor_->plane()->plane(), point );

    ac_->setDimLineRadius( arma::norm(pip - ac_->centerPoint()->value(), 2) );

    editor_->invalidate();
    editor_.updateActors();

    return true;
}




bool IQVTKDragAngleDimensionAction::onLeftButtonUp(
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    bool lastClickWasDoubleClick )
{
    finishAction();
    return true;
}
