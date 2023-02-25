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

    arma::mat pointInPlane(const QPoint& screenPos) const;

    insight::cad::SketchPointPtr
    sketchPointAtCursor( const QPoint& cp ) const;

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

  insight::cad::SketchPointPtr p1_, p2_;
  vtkSmartPointer<vtkActor> previewLine_;

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
};

#endif // IQVTKCADMODEL3DVIEWERDRAWLINE_H
