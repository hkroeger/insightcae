#ifndef IQVTKSPLITLINE_H
#define IQVTKSPLITLINE_H

#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerplanepointbasedaction.h"
#include "cadfeatures/line.h"

class IQVTKSplitLine
    : public IQVTKCADModel3DViewerPlanePointBasedAction
{
    Q_OBJECT

public:
    IQVTKSplitLine(
        IQVTKConstrainedSketchEditor &editor );

    void start() override;

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

Q_SIGNALS:
    void splitPointSelected(
        PointProperty*  addedPoint,
        std::shared_ptr<insight::cad::Line> originalLine
        );
    void splitLineAdded(
        std::shared_ptr<insight::cad::Line> addedLine,
        std::shared_ptr<insight::cad::Line> originalLine
        );
};

#endif // IQVTKSPLITLINE_H
