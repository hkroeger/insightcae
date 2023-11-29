#ifndef IQVTKCADMODEL3DVIEWERDRAWPOINT_H
#define IQVTKCADMODEL3DVIEWERDRAWPOINT_H

#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerplanepointbasedaction.h"


class IQVTKCADModel3DViewerDrawPoint
    : public IQVTKCADModel3DViewerPlanePointBasedAction
{
    Q_OBJECT


public:
    IQVTKCADModel3DViewerDrawPoint(
        IQVTKConstrainedSketchEditor &editor );

    void start() override;

    bool onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

Q_SIGNALS:
    void pointAdded(
        IQVTKCADModel3DViewerDrawPoint::PointProperty
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWPOINT_H
