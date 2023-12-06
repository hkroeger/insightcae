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

    friend class SketchEntityMultiSelection;
    friend class IQVTKSelectConstrainedSketchEntity;

private:
    typedef
        std::map<
            insight::cad::ConstrainedSketchEntityPtr,
            ActorSet >
        SketchGeometryActorMap;

    IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance_;

    SketchGeometryActorMap sketchGeometryActors_;

    insight::ParameterSet defaultGeometryParameters_;

    QToolBar *toolBar_;

    std::unique_ptr<IQVTKCADModel3DViewer::ExposeItem> transparency_;

    std::set<std::string> hiddenLayers_;


    void add(insight::cad::ConstrainedSketchEntityPtr);
    void remove(insight::cad::ConstrainedSketchEntityPtr);

    bool defaultSelectionActionRunning();

protected:
    void launchChildAction(Ptr childAction) override;

private Q_SLOTS:
    void launchDefaultSelectionAction();
    void drawPoint();
    void drawLine();
    void drawRectangle();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            insight::cad::ConstrainedSketchPtr sketch,
            const insight::ParameterSet& defaultGeometryParameters,
            IQCADModel3DViewer::SetSketchEntityAppearanceCallback setActorAppearance
            );
    ~IQVTKConstrainedSketchEditor();

    void start() override;

    insight::cad::ConstrainedSketchEntityPtr
        findSketchElementOfActor(vtkProp *actor) const;

    void deleteEntity(std::weak_ptr<insight::cad::ConstrainedSketchEntity> td);
    bool layerIsVisible(const std::string &layerName) const;

    bool onLeftButtonDoubleClick  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
    bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key ) override;
    bool onMouseMove
      (
       Qt::MouseButtons buttons,
       const QPoint point,
       Qt::KeyboardModifiers curFlags
       ) override;


public Q_SLOTS:
    void updateActors();
    void hideLayer(const std::string& layerName);
    void showLayer(const std::string& layerName);
    void renameLayer(const std::string& currentLayerName, const std::string& newLayerName);

Q_SIGNALS:
    void sketchChanged();

};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
