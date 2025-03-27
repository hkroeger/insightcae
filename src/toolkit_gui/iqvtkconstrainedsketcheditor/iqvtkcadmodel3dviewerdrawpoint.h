#ifndef IQVTKCADMODEL3DVIEWERDRAWPOINT_H
#define IQVTKCADMODEL3DVIEWERDRAWPOINT_H

#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerplanepointbasedaction.h"


class IQVTKCADModel3DViewerDrawPoint
    : public IQVTKCADModel3DViewerPlanePointBasedAction
{
    Q_OBJECT


public:
    IQVTKCADModel3DViewerDrawPoint(
        IQVTKConstrainedSketchEditor &editor );

    void start() override;

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

Q_SIGNALS:
    void pointAdded(
        IQVTKCADModel3DViewerDrawPoint::PointProperty
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWPOINT_H
