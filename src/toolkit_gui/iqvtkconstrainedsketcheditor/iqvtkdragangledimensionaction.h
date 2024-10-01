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
    ~IQVTKDragAngleDimensionAction();

    void start() override;

    bool onMouseMove
        (
            Qt::MouseButtons buttons,
            const QPoint point,
            Qt::KeyboardModifiers curFlags
            ) override;

    bool onLeftButtonUp(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;
};

#endif // IQVTKDRAGANGLEDIMENSIONACTION_H
