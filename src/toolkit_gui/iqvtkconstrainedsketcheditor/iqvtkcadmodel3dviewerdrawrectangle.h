#ifndef IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H
#define IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H


#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerplanepointbasedaction.h"
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
    bool addToSketch_;

public:
    boost::signals2::signal<void(const arma::mat& p1, const arma::mat& p2)> previewUpdated;

private:
    std::pair<arma::mat, arma::mat>
    calcP2P4(const arma::mat& p2_3d) const;

    void updatePreviewRect(const arma::mat& point3d);


public:
    IQVTKCADModel3DViewerDrawRectangle(
        IQVTKConstrainedSketchEditor &editor,
        bool allowExistingPoints = true,
        bool addToSketch = true );
    ~IQVTKCADModel3DViewerDrawRectangle();

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

Q_SIGNALS:
    void updateActors();
    void rectangleAdded(
        std::vector<std::shared_ptr<insight::cad::Line> > addedLines,
        PointProperty* p2,
        PointProperty* p1
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWRECTANGLE_H
