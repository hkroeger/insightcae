#ifndef IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H
#define IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H


#include "iqcadmodel3dviewer/iqvtkvieweractions/iqvtkcadmodel3dviewerplanepointbasedaction.h"
#include "constrainedsketch.h"
#include "cadfeatures/line.h"


class IQVTKCADModel3DViewerDrawRectangle
    : public IQVTKCADModel3DViewerPlanePointBasedAction
{
    Q_OBJECT


private:
    std::shared_ptr<PointProperty> p1_, p2_;

    vtkSmartPointer<vtkActor> previewLine_;
    insight::cad::Line* prevLine_;

private:
    std::pair<arma::mat, arma::mat>
    calcP2P4(const arma::mat& p2_3d) const;

    void updatePreviewRect(const arma::mat& point3d);


public:
    IQVTKCADModel3DViewerDrawRectangle(
        IQVTKConstrainedSketchEditor &editor );
    ~IQVTKCADModel3DViewerDrawRectangle();

    void start() override;

    bool onMouseMove
        (
            Qt::MouseButtons buttons,
            const QPoint point,
            Qt::KeyboardModifiers curFlags
            ) override;

    bool onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

Q_SIGNALS:
    void updateActors();
    void rectangleAdded(
        std::vector<std::shared_ptr<insight::cad::Line> > addedLines,
        PointProperty* p2,
        PointProperty* p1
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H
