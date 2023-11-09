#ifndef IQVTKDRAGDIMENSIONLINEACTION_H
#define IQVTKDRAGDIMENSIONLINEACTION_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor.h"
#include "cadpostprocactions/pointdistance.h"

class IQVTKCADModel3DViewer;

class IQVTKDragDimensionlineAction
: public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    IQVTKConstrainedSketchEditor &editor_;
    std::shared_ptr<insight::cad::DistanceConstraint> dc_;

public:
    IQVTKDragDimensionlineAction(
        IQVTKConstrainedSketchEditor &editor,
        std::shared_ptr<insight::cad::DistanceConstraint> dc);
    ~IQVTKDragDimensionlineAction();

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

#endif // IQVTKDRAGDIMENSIONLINEACTION_H
