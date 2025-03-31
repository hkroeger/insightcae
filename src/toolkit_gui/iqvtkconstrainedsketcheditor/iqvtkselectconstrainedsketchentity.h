#ifndef IQVTKSELECTCONSTRAINEDSKETCHENTITY_H
#define IQVTKSELECTCONSTRAINEDSKETCHENTITY_H

#include <QObject>


#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/selectionlogic.h"



class ParameterEditorWidget;
class IQVTKConstrainedSketchEditor;


typedef
    std::set<
        std::weak_ptr<insight::cad::ConstrainedSketchEntity>,
        std::owner_less<std::weak_ptr<insight::cad::ConstrainedSketchEntity> > >

        SketchEntityMultiSelectionSet;




class SketchEntityMultiSelection
    : public QObject,
      public SketchEntityMultiSelectionSet
{
    Q_OBJECT

    IQVTKConstrainedSketchEditor& editor_;

    QWidget *pew_;
    ParameterEditorWidget* pe_;

    void showPropertiesEditor(bool includeParameterEditor);
    void removePropertiesEditor();

public:
    SketchEntityMultiSelection(IQVTKConstrainedSketchEditor& editor);
    ~SketchEntityMultiSelection();

    void insert(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
    void erase(std::weak_ptr<insight::cad::ConstrainedSketchEntity> entity);
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




class TOOLKIT_GUI_EXPORT IQVTKSelectConstrainedSketchEntity
    : public QObject,
      public IQVTKConstrainedSketchEditorSelectionLogic
{
    Q_OBJECT

    std::vector<std::weak_ptr<insight::cad::ConstrainedSketchEntity>> findEntitiesUnderCursor(
        const QPoint& point) const override;

    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
        std::weak_ptr<insight::cad::ConstrainedSketchEntity>, QColor hicol ) const override;

    IQVTKConstrainedSketchEditor& editor_;
    QToolBar *toolBar_;

public:
    IQVTKSelectConstrainedSketchEntity(IQVTKConstrainedSketchEditor& editor);
    ~IQVTKSelectConstrainedSketchEntity();

    void start() override;

    bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

    IQVTKConstrainedSketchEditor& editor() const;
    insight::cad::ConstrainedSketch& sketch() const;
};



#endif // IQVTKSELECTCONSTRAINEDSKETCHENTITY_H
