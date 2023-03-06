#ifndef IQVTKCONSTRAINEDSKETCHEDITOR_H
#define IQVTKCONSTRAINEDSKETCHEDITOR_H

#include "insightcad_gui_export.h"

#include "vtkVersionMacros.h"
#include "vtkActor.h"

#include <QObject>
#include "QToolBar"

#include "sketch.h"
//#include "iqvtkviewerstate.h"
#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class INSIGHTCAD_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public QObject,
        public ViewWidgetAction<IQVTKCADModel3DViewer>, //IQVTKViewerState,
        public insight::cad::ConstrainedSketchPtr
{
    Q_OBJECT

public:
    typedef
        std::set<vtkSmartPointer<vtkProp> >
        ActorSet;

private:
    typedef
        std::map<
            insight::cad::ConstrainedSketchGeometryPtr,
            ActorSet >
        SketchGeometryActorMap;

    SketchGeometryActorMap sketchGeometryActors_;

    void add(insight::cad::ConstrainedSketchGeometryPtr);
    void remove(insight::cad::ConstrainedSketchGeometryPtr);

    QToolBar *toolBar_;

    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentAction_;

private Q_SLOTS:
    void drawLine();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            insight::cad::ConstrainedSketchPtr sketch
            );
    ~IQVTKConstrainedSketchEditor();

    insight::cad::ConstrainedSketchGeometryPtr
        findSketchElementOfActor(vtkActor *actor) const;


    void onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    void onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

    void onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;
    void onKeyRelease ( Qt::KeyboardModifiers modifiers, int key ) override;

    void onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;

    void onMouseWheel
      (
        double angleDeltaX,
        double angleDeltaY
       ) override;


public Q_SLOTS:
    void updateActors();

Q_SIGNALS:
    void finished();
};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
