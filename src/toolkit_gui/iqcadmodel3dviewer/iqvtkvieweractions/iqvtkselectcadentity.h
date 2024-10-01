#ifndef IQVTKSELECTCADENTITY_H
#define IQVTKSELECTCADENTITY_H

#include "toolkit_gui_export.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/selectionlogic.h"

#include "parametereditorwidget.h"

#include <QDockWidget>
#include <QToolBox>

class CADEntityMultiSelection
    : public QObject,
      public std::set<IQCADModel3DViewer::CADEntity>
{
    Q_OBJECT

    insight::ParameterSet commonParameters_, defaultCommonParameters_;
    IQVTKCADModel3DViewer& viewer_;

    QWidget *pew_;
    ParameterEditorWidget* pe_;

    void showParameterEditor();
    void removeParameterEditor();

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

    void start() override;
};



#endif // IQVTKSELECTCADENTITY_H
