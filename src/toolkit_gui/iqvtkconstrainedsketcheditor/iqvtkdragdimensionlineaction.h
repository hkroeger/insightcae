#ifndef IQVTKDRAGDIMENSIONLINEACTION_H
#define IQVTKDRAGDIMENSIONLINEACTION_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor.h"
#include "constrainedsketchentities/distanceconstraint.h"

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

    void start() override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;

};

#endif // IQVTKDRAGDIMENSIONLINEACTION_H
