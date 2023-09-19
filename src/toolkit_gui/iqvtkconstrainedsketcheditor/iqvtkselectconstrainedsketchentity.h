#ifndef IQVTKSELECTCONSTRAINEDSKETCHENTITY_H
#define IQVTKSELECTCONSTRAINEDSKETCHENTITY_H

#include <QObject>


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

    IQVTKConstrainedSketchEditor& editor() const;
    insight::cad::ConstrainedSketch& sketch() const;
};



#endif // IQVTKSELECTCONSTRAINEDSKETCHENTITY_H
