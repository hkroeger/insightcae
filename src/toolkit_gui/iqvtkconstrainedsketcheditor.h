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
//#include "iqvtkviewerstate.h"
#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/selectionlogic.h"


class ParameterEditorWidget;
class IQVTKConstrainedSketchEditor;


typedef
    std::map<
        std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
        std::set<std::weak_ptr<IQVTKViewerState>,
                 std::owner_less<std::weak_ptr<IQVTKViewerState> > >,
        std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > >

        SketchEntityMultiSelectionMap;


class SketchEntityMultiSelection
    : public QObject,
      public SketchEntityMultiSelectionMap
{
    Q_OBJECT

    insight::ParameterSet commonParameters_, defaultCommonParameters_;
    IQVTKConstrainedSketchEditor& editor_;
    ParameterEditorWidget* pe_;
    int tbi_;

public:
    SketchEntityMultiSelection(IQVTKConstrainedSketchEditor& editor);
    ~SketchEntityMultiSelection();

    void insert(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
    void erase(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
    bool isInSelection(const insight::cad::ConstrainedSketchEntity* entity);
};


typedef
    SelectionLogic<
        ViewWidgetAction<IQVTKCADModel3DViewer>,
        IQVTKCADModel3DViewer,
        std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
        IQVTKCADModel3DViewer::HighlightingHandleSet,
        SketchEntityMultiSelection,
        std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
    >

    IQVTKConstrainedSketchEditorSelectionLogic;




class TOOLKIT_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public QWidget, //QObject,
        public IQVTKConstrainedSketchEditorSelectionLogic, // ViewWidgetAction<IQVTKCADModel3DViewer> //IQVTKViewerState,
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

//    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentAction_;

    friend class SketchEntityMultiSelection;

    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity> >
    findEntitiesUnderCursor(const QPoint& point) const override;

//    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
//        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity
//        ) const override;
//    void unhighlightEntity(
//        IQVTKCADModel3DViewer::HighlightingHandleSet highlighters
//        ) const override;
    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
        std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity,
        QColor hicol
        ) const override;
//    void unhighlightEntity(
//        std::weak_ptr<insight::cad::ConstrainedSketchEntity> highlighters
//        ) const override;

//    std::shared_ptr<SketchEntitySelection> currentSelection_;

private Q_SLOTS:
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


//    bool onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
//    bool onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
//    bool onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
//    bool onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
//    bool onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
//    bool onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;

//    bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;
    bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key ) override;

    void onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;

//    void onMouseWheel
//      (
//        double angleDeltaX,
//        double angleDeltaY
//       ) override;


public Q_SLOTS:
    void updateActors(int update_msec=100);

Q_SIGNALS:
    void finished();
};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
