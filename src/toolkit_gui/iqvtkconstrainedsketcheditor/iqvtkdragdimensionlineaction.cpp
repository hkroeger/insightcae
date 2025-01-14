#include "iqvtkdragdimensionlineaction.h"

#include "datum.h"


IQVTKDragDimensionlineAction::IQVTKDragDimensionlineAction(
    IQVTKConstrainedSketchEditor &editor,
    std::shared_ptr<insight::cad::DistanceConstraint> dc )
    : ViewWidgetAction<IQVTKCADModel3DViewer>(editor.viewer(), true),
    editor_(editor), dc_(dc)
{
}


IQVTKDragDimensionlineAction::~IQVTKDragDimensionlineAction()
{}



void IQVTKDragDimensionlineAction::start()
{}




bool IQVTKDragDimensionlineAction::onMouseMove
    (
        Qt::MouseButtons buttons,
        const QPoint point,
        Qt::KeyboardModifiers curFlags
        )
{
    arma::mat pip=viewer().pointInPlane3D(
        editor_->plane()->plane(), point );

    dc_->setDimLineOffset(pip);

    editor_->invalidate();
    editor_.updateActors();

    return true;
}




bool IQVTKDragDimensionlineAction::onLeftButtonUp(
    Qt::KeyboardModifiers nFlags,
    const QPoint point,
    bool lastClickWasDoubleClick )
{
    finishAction();
    return true;
}
