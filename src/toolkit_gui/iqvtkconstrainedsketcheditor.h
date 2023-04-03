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

#include "sketch.h"
//#include "iqvtkviewerstate.h"
#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;
class ParameterEditorWidget;


class IQVTKConstrainedSketchEntity
        : public insight::cad::ConstrainedSketchEntity
{
public:
    virtual std::vector<vtkSmartPointer<vtkProp> > createActor() const =0;
};





class IQVTKFixedPoint
        : public IQVTKConstrainedSketchEntity
{

    insight::cad::SketchPointPtr p_;

public:
    IQVTKFixedPoint(
            insight::cad::SketchPointPtr p  );

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
};


class IQVTKHorizontalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line const> line_;

public:
    IQVTKHorizontalConstraint(std::shared_ptr<insight::cad::Line const> line);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
};


class IQVTKVerticalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line const> line_;
public:
    IQVTKVerticalConstraint(std::shared_ptr<insight::cad::Line const> line);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
};





class TOOLKIT_GUI_EXPORT IQVTKConstrainedSketchEditor
      : public QWidget, //QObject,
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
            insight::cad::ConstrainedSketchEntityPtr,
            ActorSet >
        SketchGeometryActorMap;

    SketchGeometryActorMap sketchGeometryActors_;

    void add(insight::cad::ConstrainedSketchEntityPtr);
    void remove(insight::cad::ConstrainedSketchEntityPtr);

    insight::ParameterSet defaultGeometryParameters_;

    QToolBar *toolBar_;
    QDockWidget *toolBoxWidget_;
    QToolBox *toolBox_;

    ViewWidgetAction<IQVTKCADModel3DViewer>::Ptr currentAction_;

    struct SketchEntitySelectionViewPropsToRestore
    {
        double oldColor[3];
    };
    class SketchEntitySelection
     : public std::map<
            std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
            std::map<vtkProp*, SketchEntitySelectionViewPropsToRestore>,
            std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > >
    {
        insight::ParameterSet commonParameters_, defaultCommonParameters_;
        IQVTKConstrainedSketchEditor& editor_;
        ParameterEditorWidget* pe_;
        int tbi_;

        void highlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
        void unhighlight(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);

    public:
        SketchEntitySelection(IQVTKConstrainedSketchEditor& editor);
        ~SketchEntitySelection();
        void addAndHighlight(insight::cad::ConstrainedSketchEntityPtr entity);
        bool isInSelection(const insight::cad::ConstrainedSketchEntityPtr& entity);
    };
    friend class SketchEntitySelection;

    std::shared_ptr<SketchEntitySelection> currentSelection_;

private Q_SLOTS:
    void drawLine();
    void solve();

public:
    IQVTKConstrainedSketchEditor(
            IQVTKCADModel3DViewer& viewer,
            insight::cad::ConstrainedSketchPtr sketch,
            const insight::ParameterSet& defaultGeometryParameters
            );
    ~IQVTKConstrainedSketchEditor();

    insight::cad::ConstrainedSketchEntityPtr
        findSketchElementOfActor(vtkProp *actor) const;


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
