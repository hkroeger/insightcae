#ifndef IQVTKSELECTCADENTITY_H
#define IQVTKSELECTCADENTITY_H

#include "toolkit_gui_export.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/selectionlogic.h"

#include "parametereditorwidget.h"

#include <QDockWidget>
#include <QToolBox>

#include <map>

class CADEntityMultiSelection
    : public QObject,
      public std::set<IQCADModel3DViewer::CADEntity>
{
    Q_OBJECT

    struct TopLevelEntry { std::string label; std::string absPath; };

    IQVTKCADModel3DViewer& viewer_;

    QWidget *editorContainerWidget_;
    ParameterEditorWidget* editorWidget_;

    // Current entity1 top-level entries that are common to all selected entities
    std::vector<TopLevelEntry> currentEntries_;
    // entity1 absPath -> [entity2 absPath, entity3 absPath, ...] (for copy-on-change)
    std::map<std::string, std::vector<std::string>> copyMapping_;
    // guard: prevent recursive copy
    bool copyingInProgress_ = false;
    // handle for the copy-on-change connection
    QMetaObject::Connection copyConnection_;

    void showParameterEditor();
    void removeParameterEditor();
    void rebuildEditor();

    std::vector<std::string> getParamListForFeature(const insight::cad::FeaturePtr& feat) const;
    std::vector<TopLevelEntry> getTopLevelEntries(const std::vector<std::string>& assocParamPaths,
                                                   QAbstractItemModel* apsm) const;

public:
    CADEntityMultiSelection(IQVTKCADModel3DViewer& viewer);
    ~CADEntityMultiSelection();

    void insert(IQCADModel3DViewer::CADEntity entity);
    void erase(IQCADModel3DViewer::CADEntity entity);
};



typedef
    SelectionLogic<
        ViewWidgetAction<IQVTKCADModel3DViewer>,
        IQVTKCADModel3DViewer,
        IQCADModel3DViewer::CADEntity,
        IQVTKCADModel3DViewer::HighlightingHandleSet,
        CADEntityMultiSelection
    >

    IQVTKCADModel3DViewerSelectionLogic;




class TOOLKIT_GUI_EXPORT IQVTKSelectCADEntity
: public QObject,
  public IQVTKCADModel3DViewerSelectionLogic
{
    Q_OBJECT

    std::vector<IQCADModel3DViewer::CADEntity> findEntitiesUnderCursor(
        const QPoint& point) const override;

    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
        IQCADModel3DViewer::CADEntity entity, QColor hicol ) const override;

    QToolBar *toolBar_;

public:
    IQVTKSelectCADEntity(IQVTKCADModel3DViewer& viewer);
    ~IQVTKSelectCADEntity();

    QString description() const override;

    bool onMouseClick(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point ) override;

    bool onKeyPress(Qt::KeyboardModifiers modifiers, int key) override;

    void start() override;
};




typedef
    SelectionLogic<
        ViewWidgetAction<IQVTKCADModel3DViewer>,
        IQVTKCADModel3DViewer,
        IQVTKCADModel3DViewer::SubshapeData,
        IQVTKCADModel3DViewer::HighlightingHandleSet
        >

        IQVTKCADModel3DViewerSubshapeSelectionLogic;




class TOOLKIT_GUI_EXPORT IQVTKSelectSubshape
    : public QObject,
      public IQVTKCADModel3DViewerSubshapeSelectionLogic
{
    Q_OBJECT

    std::vector<IQVTKCADModel3DViewer::SubshapeData> findEntitiesUnderCursor(
        const QPoint& point) const override;

    IQVTKCADModel3DViewer::HighlightingHandleSet highlightEntity(
        IQVTKCADModel3DViewer::SubshapeData entity, QColor hicol ) const override;


public:
    IQVTKSelectSubshape(IQVTKCADModel3DViewer& viewer);
    ~IQVTKSelectSubshape();

    QString description() const override;

    void start() override;
};



#endif // IQVTKSELECTCADENTITY_H
