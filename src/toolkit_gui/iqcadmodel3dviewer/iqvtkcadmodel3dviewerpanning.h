#ifndef IQVTKCADMODEL3DVIEWERPANNING_H
#define IQVTKCADMODEL3DVIEWERPANNING_H


#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;


class IQVTKCADModel3DViewerPanning
    : public ViewWidgetAction<IQVTKCADModel3DViewer>
{

    void ComputeWorldToDisplay(
      double x, double y, double z, double displayPt[3]);

    void ComputeDisplayToWorld(
            double x, double y, double z, double worldPt[4]);

    void pan(const QPoint& point);

public:
  IQVTKCADModel3DViewerPanning(IQVTKCADModel3DViewer &viewWidget, const QPoint point);

  void start() override;

  bool onMouseDrag  (
      Qt::MouseButtons btn,
      Qt::KeyboardModifiers nFlags,
      const QPoint point,
      EventType eventType ) override;

  bool onMouseMove(
      const QPoint point,
      Qt::KeyboardModifiers curFlags ) override;
};

#endif // IQCADMODEL3DVIEWERPANNING_H
