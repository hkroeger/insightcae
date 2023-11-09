#ifndef IQVTKCADMODEL3DVIEWERDRAWLINE_H
#define IQVTKCADMODEL3DVIEWERDRAWLINE_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor/iqvtkselectconstrainedsketchentity.h"

#include "constrainedsketch.h"
#include "cadfeatures/line.h"

#include "vtkActor.h"

class IQVTKCADModel3DViewer;




class IQVTKCADModel3DViewerPlanePointBasedAction
      : public IQVTKSelectConstrainedSketchEntity
{

protected:
    boost::signals2::signal<void(insight::cad::SketchPointPtr)> existingPointSelected;
    boost::signals2::signal<void(insight::cad::SketchPointPtr)> newPointCreated;

public:
    IQVTKCADModel3DViewerPlanePointBasedAction(
            IQVTKConstrainedSketchEditor &editor );

    bool onLeftButtonDown(
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;
};





class IQVTKCADModel3DViewerDrawLine
        : public IQVTKCADModel3DViewerPlanePointBasedAction
{
  Q_OBJECT


public:
  struct EndPointProperty
  {
      insight::cad::SketchPointPtr p;
      bool isAnExistingPoint;
      insight::cad::FeaturePtr onFeature;
  };

private:
  std::shared_ptr<EndPointProperty> p1_, p2_;
  vtkSmartPointer<vtkActor> previewLine_;

  insight::cad::Line* prevLine_;

private:
  /**
   * @brief pcand_
   * left click might change viewport geometry due to modification of property panel
   * determine candidate already during mouse move and use it as displayed
   */


  void updatePreviewLine(const arma::mat& point3d);

  insight::cad::SketchPointPtr applyWizards(const QPoint screenPoint) const;
  insight::cad::SketchPointPtr applyWizards(const arma::mat& pip3d) const;

  void setP1(insight::cad::SketchPointPtr p1, bool isExistingPoint);
  void setP2(insight::cad::SketchPointPtr p2, bool isExistingPoint);

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
        EndPointProperty*  addedPoint,
        insight::cad::SketchPointPtr previousPoint
        );
    void lineAdded(
        insight::cad::Line* addedLine,
        insight::cad::Line* previouslyAddedLine,
        EndPointProperty* p2,
        EndPointProperty* p1
        );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWLINE_H
