#ifndef IQVTKDRAGPOINT_H
#define IQVTKDRAGPOINT_H


#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor.h"

class IQVTKCADModel3DViewer;

class IQVTKDragPoint
    : public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    IQVTKConstrainedSketchEditor &editor_;
    std::shared_ptr<insight::cad::SketchPoint> p_;

public:
    IQVTKDragPoint(
        IQVTKConstrainedSketchEditor &editor,
        std::shared_ptr<insight::cad::SketchPoint> p);

    void start() override;

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType ) override;
};



#endif // IQVTKDRAGPOINT_H
