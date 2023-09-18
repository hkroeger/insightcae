#ifndef IQVTKCADMODEL3DVIEWERDRAWLINE_H
#define IQVTKCADMODEL3DVIEWERDRAWLINE_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"

#include "constrainedsketch.h"
#include "cadfeatures/line.h"

#include "vtkActor.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerPlanePointBasedAction
        : public QObject,
        public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    Q_OBJECT

protected:
    insight::cad::ConstrainedSketchPtr sketch_;

    insight::cad::SketchPointPtr existingSketchPointAt(
        const QPoint& cp, insight::cad::ConstrainedSketchEntityPtr& sg ) const;

Q_SIGNALS:
    void updateActors();
    void finished();

public:
    IQVTKCADModel3DViewerPlanePointBasedAction(
            IQVTKCADModel3DViewer &viewWidget,
            insight::cad::ConstrainedSketchPtr sketch );
};



class IQVTKCADModel3DViewerDrawLine
        : public IQVTKCADModel3DViewerPlanePointBasedAction
{
  Q_OBJECT

  insight::cad::SketchPointPtr p1_, p2_;
  vtkSmartPointer<vtkActor> previewLine_;

  insight::cad::Line* prevLine_;

public:
  struct CandidatePoint
  {
      insight::cad::SketchPointPtr sketchPoint;
      bool isAnExistingPoint;
      insight::cad::FeaturePtr onFeature;
  };

private:
  /**
   * @brief pcand_
   * left click might change viewport geometry due to modification of property panel
   * determine candidate already during mouse move and use it as displayed
   */
  std::unique_ptr<CandidatePoint> p1cand_, pcand_;

  CandidatePoint updatePCand(
      const QPoint& point ) const;

public:
    IQVTKCADModel3DViewerDrawLine(
            IQVTKCADModel3DViewer &viewWidget,
            insight::cad::ConstrainedSketchPtr sketch );
    ~IQVTKCADModel3DViewerDrawLine();

    void onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;

    bool onLeftButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onRightButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

Q_SIGNALS:
    void endPointSelected(
        CandidatePoint* addPoint,
        insight::cad::SketchPointPtr previousPoint
        );
    void lineAdded(
        insight::cad::Line* addedLine,
        insight::cad::Line* previouslyAddedLine,
        CandidatePoint* p1, CandidatePoint* p2 );
};

#endif // IQVTKCADMODEL3DVIEWERDRAWLINE_H
