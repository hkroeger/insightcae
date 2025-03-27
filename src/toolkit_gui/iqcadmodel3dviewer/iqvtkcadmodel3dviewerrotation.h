#ifndef IQVTKCADMODEL3DVIEWERROTATION_H
#define IQVTKCADMODEL3DVIEWERROTATION_H


#include "viewwidgetaction.h"

class IQVTKCADModel3DViewer;

class IQVTKCADModel3DViewerRotation
    : public ViewWidgetAction<IQVTKCADModel3DViewer>
{

    const double MotionFactor = 10.;
    const bool AutoAdjustCameraClippingRange = true;

    void rotate(const QPoint& point);

public:
  IQVTKCADModel3DViewerRotation(
        IQVTKCADModel3DViewer &viewWidget, const QPoint point);

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


#endif // IQCADMODEL3DVIEWERROTATION_H
