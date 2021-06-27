#ifndef VIEWWIDGETACTION_H
#define VIEWWIDGETACTION_H

#include "insightcad_gui_export.h"


#include "cadtypes.h"
#include "feature.h"


#include <memory>

#include <QWidget>
#include <QRubberBand>

#include "Standard_Version.hxx"
#include "V3d_View.hxx"
#include "V3d_Plane.hxx"
#include "AIS_InteractiveContext.hxx"
#include "AIS_Plane.hxx"
#include "V3d_Coordinate.hxx"
#if OCC_VERSION_MAJOR>=7
#include "AIS_ViewController.hxx"

class Aspect_GraphicCallbackStruct;
#endif


class QoccViewWidget;

class InputReceiver
{
protected:
  /**
   * @brief currentPoint_
   * Track mouse location for use in key events.
   * Attention: invalid until first mouse event received!
   */
  QPoint lastMouseLocation_;

public:
  const QPoint& lastMouseLocation() const;
  virtual void onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point );
  virtual void onKeyPress ( Qt::KeyboardModifiers modifiers, int key );
  virtual void onKeyRelease ( Qt::KeyboardModifiers modifiers, int key );
  virtual void onMouseMove
    (
     Qt::MouseButtons buttons,
     Qt::KeyboardModifiers nFlags,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     );
  virtual void onMouseWheel
    (
      double angleDeltaX,
      double angleDeltaY
     );
};



class ViewWidgetAction : public InputReceiver
{
  bool finished_ = false;

protected:
  virtual void setFinished();

public:
  ViewWidgetAction();
  virtual ~ViewWidgetAction();

  inline bool finished() const { return finished_; }
};

typedef std::shared_ptr<ViewWidgetAction> ViewWidgetActionPtr;



class ViewWidgetRotation : public ViewWidgetAction
{
  Handle_V3d_View view_;

public:
  ViewWidgetRotation(Handle_V3d_View view, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     Qt::KeyboardModifiers nFlags,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};


class ViewWidgetPanning : public ViewWidgetAction
{
  Handle_V3d_View view_;
  QPoint startPoint_;

public:
  ViewWidgetPanning(Handle_V3d_View view, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     Qt::KeyboardModifiers nFlags,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};


class ViewWidgetDynamicZooming : public ViewWidgetAction
{
  Handle_V3d_View view_;
  QPoint startPoint_;

public:
  ViewWidgetDynamicZooming(Handle_V3d_View view, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     Qt::KeyboardModifiers nFlags,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};



class ViewWidgetWindowZooming : public ViewWidgetAction
{
  Handle_V3d_View view_;
  QPoint startPoint_;
  QRubberBand *rb_;

public:
  ViewWidgetWindowZooming(Handle_V3d_View view, const QPoint point, QRubberBand* rb);
  ~ViewWidgetWindowZooming();

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     Qt::KeyboardModifiers nFlags,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};






class ViewWidgetMeasurePoints : public ViewWidgetAction
{
  QoccViewWidget *viewWidget_;
  std::shared_ptr<insight::cad::Vector> p1_, p2_;

public:
  ViewWidgetMeasurePoints(QoccViewWidget *viewWidget);
  ~ViewWidgetMeasurePoints();

  void onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
};



#endif // VIEWWIDGETACTION_H
