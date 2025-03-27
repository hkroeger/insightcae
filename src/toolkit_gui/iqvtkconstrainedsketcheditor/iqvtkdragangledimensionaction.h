#ifndef IQVTKDRAGANGLEDIMENSIONACTION_H
#define IQVTKDRAGANGLEDIMENSIONACTION_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor.h"
#include "constrainedsketchentities/angleconstraint.h"

class IQVTKCADModel3DViewer;

class IQVTKDragAngleDimensionAction
    : public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    IQVTKConstrainedSketchEditor &editor_;
    std::shared_ptr<insight::cad::AngleConstraint> ac_;

public:
    IQVTKDragAngleDimensionAction(
        IQVTKConstrainedSketchEditor &editor,
        std::shared_ptr<insight::cad::AngleConstraint> ac);

    void start() override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;

    // bool onLeftButtonUp(
    //     Qt::KeyboardModifiers nFlags,
    //     const QPoint point,
    //     bool lastClickWasDoubleClick ) override;
};

#endif // IQVTKDRAGANGLEDIMENSIONACTION_H
