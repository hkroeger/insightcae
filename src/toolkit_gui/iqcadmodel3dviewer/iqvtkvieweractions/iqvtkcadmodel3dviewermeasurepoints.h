#ifndef IQCADMODEL3DVIEWERMEASUREPOINTS_H
#define IQCADMODEL3DVIEWERMEASUREPOINTS_H

#include "iqcadmodel3dviewer/viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerMeasurePoints
        : public ViewWidgetAction<IQVTKCADModel3DViewer>
{
  std::shared_ptr<insight::cad::Vector> p1_, p2_;

public:
  IQVTKCADModel3DViewerMeasurePoints(IQVTKCADModel3DViewer &viewWidget);
  ~IQVTKCADModel3DViewerMeasurePoints();
};


#endif // IQCADMODEL3DVIEWERMEASUREPOINTS_H
