#ifndef IQVTKCADMODEL3DVIEWERDRAWLINE_H
#define IQVTKCADMODEL3DVIEWERDRAWLINE_H

#include "viewwidgetaction.h"

#include "sketch.h"

#include "vtkActor.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerPlanePointBasedAction
        : public QObject,
        public ViewWidgetAction<IQVTKCADModel3DViewer>
{
    Q_OBJECT

protected:
    insight::cad::ConstrainedSketchPtr sketch_;

    arma::mat pointInPlane3D(const arma::mat& pip2d) const;
    arma::mat pointInPlane3D(const QPoint& screenPos) const;
    arma::mat pointInPlane2D(const QPoint& screenPos) const;
    arma::mat pointInPlane2D(const arma::mat& pip3d) const;

    insight::cad::SketchPointPtr
    sketchPointAtCursor(
            const QPoint& cp,
            boost::optional<arma::mat> forcedLocation = boost::optional<arma::mat>() ) const;

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
  boost::optional<arma::mat> modifiedP2_;
  vtkSmartPointer<vtkActor> previewLine_;

  insight::cad::Line* prevLine_;


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

    void onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onRightButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

Q_SIGNALS:
    void lineAdded(insight::cad::Line* addedLine, insight::cad::Line* previouslyAddedLine);
};

#endif // IQVTKCADMODEL3DVIEWERDRAWLINE_H
