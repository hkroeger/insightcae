#ifndef VIEWWIDGETACTION_H
#define VIEWWIDGETACTION_H

#include "toolkit_gui_export.h"


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

class ToNotepadEmitter
        : public QObject
{
    Q_OBJECT

public:
    ToNotepadEmitter();
    virtual ~ToNotepadEmitter();

Q_SIGNALS:
    void appendToNotepad(const QString& text);
};


template<class Viewer>
class InputReceiver
{
public:
  typedef std::shared_ptr<InputReceiver> Ptr;

private:
  Viewer& viewer_;

protected:
  /**
   * @brief currentPoint_
   * Track mouse location for use in key events.
   * Attention: invalid until first mouse event received!
   */
  std::unique_ptr<QPoint> lastMouseLocation_;

public:
  InputReceiver(Viewer& viewer)
  : viewer_(viewer)
  {}

  InputReceiver(Viewer& viewer, const QPoint& p)
  : viewer_(viewer),
    lastMouseLocation_(new QPoint(p))
  {}

  virtual ~InputReceiver()
  {}

  Viewer& viewer() const
  {
      return viewer_;
  }

  bool hasLastMouseLocation() const
  {
      return bool(lastMouseLocation_);
  }

  const QPoint& lastMouseLocation() const
  {
      insight::assertion(
                  bool(lastMouseLocation_),
                  "attempt to query mouse location before first input received!");
      return *lastMouseLocation_;
  }

  virtual bool onLeftButtonDoubleClick  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return false;
  }

  virtual bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
  {
      return false;
  }

  virtual bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
  {
      return false;
  }

  virtual void onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     )
  {
      lastMouseLocation_.reset(new QPoint(point));
  }

  virtual void onMouseWheel
    (
      double angleDeltaX,
      double angleDeltaY
     )
  {
  }

};




template<class Viewer>
class ViewWidgetAction
        : public InputReceiver<Viewer>
{
public:
  typedef std::shared_ptr<ViewWidgetAction> Ptr;

private:
  bool finished_ = false;

protected:
  virtual void setFinished()
  {
    finished_=true;
  }

public:
  ViewWidgetAction(Viewer& viewer)
      : InputReceiver<Viewer>(viewer)
  {}

  ViewWidgetAction(Viewer& viewer, const QPoint& p)
      : InputReceiver<Viewer>(viewer, p)
  {}

  inline bool finished() const { return finished_; }
};




class OCCViewWidgetRotation : public ViewWidgetAction<QoccViewWidget>
{
public:
  OCCViewWidgetRotation(QoccViewWidget &viewWidget, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};


class OCCViewWidgetPanning : public ViewWidgetAction<QoccViewWidget>
{
public:
  OCCViewWidgetPanning(QoccViewWidget &viewWidget, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};


class OCCViewWidgetDynamicZooming : public ViewWidgetAction<QoccViewWidget>
{
public:
  OCCViewWidgetDynamicZooming(QoccViewWidget &viewWidget, const QPoint point);

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};



class OCCViewWidgetWindowZooming : public ViewWidgetAction<QoccViewWidget>
{
  QRubberBand *rb_;
public:
  OCCViewWidgetWindowZooming(QoccViewWidget &viewWidget, const QPoint point, QRubberBand* rb);
  ~OCCViewWidgetWindowZooming();

  void onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     ) override;
};




class OCCViewWidgetMeasurePoints : public ViewWidgetAction<QoccViewWidget>
{
  std::shared_ptr<insight::cad::Vector> p1_, p2_;

public:
  OCCViewWidgetMeasurePoints(QoccViewWidget &viewWidget);
  ~OCCViewWidgetMeasurePoints();

  bool onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
};



#endif // VIEWWIDGETACTION_H
