#ifndef IQVTKSELECTCADENTITY_H
#define IQVTKSELECTCADENTITY_H

#include "toolkit_gui_export.h"

#include "iqvtkcadmodel3dviewer.h"
#include "iqcadmodel3dviewer/viewwidgetaction.h"
#include "iqcadmodel3dviewer/selectionlogic.h"





typedef
    SelectionLogic<
        ViewWidgetAction<IQVTKCADModel3DViewer>,
        IQVTKCADModel3DViewer,
        IQCADModel3DViewer::CADEntity,
        IQVTKCADModel3DViewer::HighlightingHandleSet
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
};



#endif // IQVTKSELECTCADENTITY_H
