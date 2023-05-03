#ifndef IQCADMODEL3DVIEWERMEASUREPOINTS_H
#define IQCADMODEL3DVIEWERMEASUREPOINTS_H

#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerMeasurePoints
        : public ViewWidgetAction<IQVTKCADModel3DViewer>
{
  std::shared_ptr<insight::cad::Vector> p1_, p2_;

public:
  IQVTKCADModel3DViewerMeasurePoints(IQVTKCADModel3DViewer &viewWidget);
  ~IQVTKCADModel3DViewerMeasurePoints();

  bool onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
};


#endif // IQCADMODEL3DVIEWERMEASUREPOINTS_H
