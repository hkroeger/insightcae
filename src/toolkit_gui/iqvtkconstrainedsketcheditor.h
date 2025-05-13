#ifndef IQVTKCONSTRAINEDSKETCHEDITOR_H
#define IQVTKCONSTRAINEDSKETCHEDITOR_H

#include "constrainedsketchentity.h"
#include "toolkit_gui_export.h"


#include "vtkVersionMacros.h"
#include "vtkSmartPointer.h"
#include "vtkActor.h"

#include <QObject>
#include <QToolBar>
#include <QToolBox>
#include <QDockWidget>
#include <memory>

#include "constrainedsketch.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqvtkconstrainedsketcheditor/iqvtkselectconstrainedsketchentity.h"
#include "iqundoredostack.h"



class DefaultGUIConstrainedSketchPresentationDelegate
    : public insight::cad::ConstrainedSketchPresentationDelegate
{
public:
    declareType("default");

    IQParameterSetModel*
    setupSketchEntityParameterSetModel(
        const insight::cad::ConstrainedSketchEntity& e) const override;

    IQParameterSetModel*
    setupLayerParameterSetModel(
        const std::string& layerName, const insight::cad::LayerProperties& e) const override;

    void setEntityAppearance(
        const insight::cad::ConstrainedSketchEntity& e, vtkProperty* actprops) const override;
};


extern std::string defaultGUIConstrainedSketchPresentationDelegate;






class TOOLKIT_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public IQUndoRedoStack,
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


    class UndoState
        : public IQUndoRedoStackState,
          public std::shared_ptr<insight::cad::ConstrainedSketch>
    {
    public:
        UndoState(
            const QString& description,
            const insight::cad::ConstrainedSketch& sk );
    };

protected:
    void applyUndoState(const IQUndoRedoStackState& state) override;
    IQUndoRedoStackStatePtr createUndoState(const QString& description) const override;

private:
    typedef
        std::map<
            insight::cad::ConstrainedSketchEntityPtr,
            ActorSet >
        SketchGeometryActorMap;


    std::unique_ptr<IQVTKCADModel3DViewer::ExposeItem> transparency_;

    SketchGeometryActorMap sketchGeometryActors_;

    std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties_;
    std::shared_ptr<insight::cad::ConstrainedSketchPresentationDelegate> presentation_;


    std::set<std::string> hiddenLayers_;

    ParameterEditorWidget* layerPropertiesEditor_;
    QWidget *sketchToolBoxWidget_;
    QToolBar *toolBar_;

    void showLayerParameterEditor();
    void hideLayerParameterEditor();
    void addActors(insight::cad::ConstrainedSketchEntityPtr);
    void removeActors(insight::cad::ConstrainedSketchEntityPtr, bool redraw=true);


    ViewWidgetActionPtr setupDefaultAction() override;

private Q_SLOTS:
    void drawPoint();
    void drawLine();
    void drawRectangle();
    void splitLine();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            const insight::cad::ConstrainedSketch& sketch,
            std::shared_ptr<insight::cad::ConstrainedSketchParametersDelegate> entityProperties,
            const std::string& presentationDelegateKey
            );
    ~IQVTKConstrainedSketchEditor();

    QString description() const override;

    void start() override;

    insight::cad::ConstrainedSketchEntityPtr
        findSketchElementOfActor(vtkProp *actor) const;

    void deleteEntity(std::weak_ptr<insight::cad::ConstrainedSketchEntity> td);
    bool layerIsVisible(const std::string &layerName) const;
    bool entityIsVisible(std::shared_ptr<insight::cad::ConstrainedSketchEntity> e) const;

    std::shared_ptr<insight::cad::ConstrainedSketchEntity>
    selectedItemUnderCursor() const;

    bool onDoubleClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

    bool onKeyRelease (
        Qt::KeyboardModifiers modifiers,
        int key ) override;

    bool onMouseDrag(
        Qt::MouseButtons buttons,
        Qt::KeyboardModifiers curFlags,
        const QPoint point,
        EventType eventType ) override;

    inline const insight::cad::ConstrainedSketch& sketch() const
    { return **this; }

    std::set<insight::cad::ConstrainedSketchEntityPtr> shownEntities() const;

public Q_SLOTS:
    void updateActors();
    void hideLayer(const std::string& layerName);
    void showLayer(const std::string& layerName);
    void renameLayer(const std::string& currentLayerName, const std::string& newLayerName);
    void onSketchSizeChanged();

Q_SIGNALS:
    void sketchChanged();

};


#endif // IQVTKCONSTRAINEDSKETCHEDITOR_H
