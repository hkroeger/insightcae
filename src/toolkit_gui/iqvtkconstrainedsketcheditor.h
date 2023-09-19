#ifndef IQVTKCONSTRAINEDSKETCHEDITOR_H
#define IQVTKCONSTRAINEDSKETCHEDITOR_H

#include "toolkit_gui_export.h"


#include "vtkVersionMacros.h"
#include "vtkSmartPointer.h"
#include "vtkActor.h"

#include <QObject>
#include <QToolBar>
#include <QToolBox>
#include <QDockWidget>

#include "constrainedsketch.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor/iqvtkselectconstrainedsketchentity.h"


class TOOLKIT_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public QWidget, //QObject,
        public ViewWidgetAction<IQVTKCADModel3DViewer>,
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
            insight::cad::ConstrainedSketchEntityPtr,
            ActorSet >
        SketchGeometryActorMap;

    IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance_;

    SketchGeometryActorMap sketchGeometryActors_;

    void add(insight::cad::ConstrainedSketchEntityPtr);
    void remove(insight::cad::ConstrainedSketchEntityPtr);

    insight::ParameterSet defaultGeometryParameters_;

    QToolBar *toolBar_;
    QDockWidget *toolBoxWidget_;
    QToolBox *toolBox_;


    friend class SketchEntityMultiSelection;
    friend class IQVTKSelectConstrainedSketchEntity;

//    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
//    findEntitiesUnderCursor(const QPoint& point) const;

//    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
//        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity,
//        QColor hicol
//        ) const;

    bool defaultSelectionActionRunning();

private Q_SLOTS:
    void launchDefaultSelectionAction();
    void drawLine();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            insight::cad::ConstrainedSketchPtr sketch,
            const insight::ParameterSet& defaultGeometryParameters,
            IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance
            );
    ~IQVTKConstrainedSketchEditor();

    insight::cad::ConstrainedSketchEntityPtr
        findSketchElementOfActor(vtkProp *actor) const;

    void deleteEntity(std::weak_ptr<insight::cad::ConstrainedSketchEntity> td);

    bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key ) override;
    void onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;


public Q_SLOTS:
    void updateActors(int update_msec=100);

Q_SIGNALS:
    void finished();
};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
