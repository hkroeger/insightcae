#ifndef IQVTKCADMODEL3DVIEWERDRAWLINE_H
#define IQVTKCADMODEL3DVIEWERDRAWLINE_H


#include "iqvtkconstrainedsketcheditor/iqvtkcadmodel3dviewerplanepointbasedaction.h"
#include "constrainedsketch.h"
#include "cadfeatures/line.h"

#include "vtkActor.h"


class IQVTKCADModel3DViewerDrawLine
        : public IQVTKCADModel3DViewerPlanePointBasedAction
{
  Q_OBJECT


private:
  std::shared_ptr<PointProperty> p1_, p2_;

  vtkSmartPointer<vtkActor> previewLine_;
  insight::cad::Line* prevLine_;

private:
  /**
   * @brief pcand_
   * left click might change viewport geometry due to modification of property panel
   * determine candidate already during mouse move and use it as displayed
   */


  void updatePreviewLine(const arma::mat& point3d);

protected:
  PointProperty applyWizards(const arma::mat& pip3d, insight::cad::FeaturePtr onFeature) const override;

public:
    IQVTKCADModel3DViewerDrawLine(
            IQVTKConstrainedSketchEditor &editor );
    ~IQVTKCADModel3DViewerDrawLine();

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
    void endPointSelected(
        PointProperty*  addedPoint,
        insight::cad::SketchPointPtr previousPoint
        );
    void lineAdded(
        std::shared_ptr<insight::cad::Line> addedLine,
        insight::cad::Line* previouslyAddedLine,
        PointProperty* p2,
        PointProperty* p1
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWLINE_H
