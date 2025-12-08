#ifndef IQVTKCADMODEL3DVIEWERDRAWARC_H
#define IQVTKCADMODEL3DVIEWERDRAWARC_H

#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerplanepointbasedaction.h"
#include "constrainedsketch.h"
#include "cadfeatures/arc.h"

#include "vtkActor.h"

class IQVTKCADModel3DViewerDrawArc
: public IQVTKCADModel3DViewerPlanePointBasedAction
{
  Q_OBJECT

private:
    std::shared_ptr<PointProperty> p1_, p2_, pm_;

    vtkSmartPointer<vtkActor> previewLine_;

    void updatePreviewLine(const arma::mat& point3d);

protected:
    PointProperty applyWizards(const arma::mat& pip3d, insight::cad::FeaturePtr onFeature) const override;

public:
    IQVTKCADModel3DViewerDrawArc(
        IQVTKConstrainedSketchEditor &editor );

    ~IQVTKCADModel3DViewerDrawArc();

    void start() override;

    bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags ) override;

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

    QString description() const override;

Q_SIGNALS:
    void endPointSelected(
        PointProperty*  addedPoint,
        insight::cad::SketchPointPtr previousPoint
        );
    void lineAdded(
        std::shared_ptr<insight::cad::Arc> addedLine,
        PointProperty* p2,
        PointProperty* p1
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWARC_H
