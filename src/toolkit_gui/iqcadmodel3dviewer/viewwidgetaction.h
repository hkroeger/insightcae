#ifndef VIEWWIDGETACTION_H
#define VIEWWIDGETACTION_H

#include "toolkit_gui_export.h"


#include "cadtypes.h"
#include "feature.h"
#include "boost/signals2.hpp"


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

  boost::signals2::signal<void()> aboutToBeDestroyed;

private:
  Viewer& viewer_;

  std::set<InputReceiver*> childReceivers_;

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
  {
      aboutToBeDestroyed();
  }

  void registerChildReceiver(InputReceiver* recv)
  {
      recv->aboutToBeDestroyed.connect(
          [this,recv]()
          {
              childReceivers_.erase(recv);
          }
          );
      childReceivers_.insert(recv);
  }

  void removeChildReceiver(InputReceiver* recv)
  {
      childReceivers_.erase(recv);
  }


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

  void updateLastMouseLocation(const QPoint& p)
  {
      lastMouseLocation_.reset(new QPoint(p));
  }

  virtual bool onLeftButtonDoubleClick  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onLeftButtonDoubleClick(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onLeftButtonDown(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onMiddleButtonDown(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onRightButtonDown(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onLeftButtonUp(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onMiddleButtonUp(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onRightButtonUp(nFlags, point)) return true;
      }
      return false;
  }

  virtual bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onKeyPress(modifiers, key)) return true;
      }
      return false;
  }

  virtual bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
  {
      for (auto&c: childReceivers_)
      {
          if (c->onKeyRelease(modifiers, key)) return true;
      }
      return false;
  }

  virtual bool onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     )
  {
      bool handled=false;
      for (auto&c: childReceivers_)
      {
          if (!handled)
          {
              handled=c->onMouseMove(buttons, point, curFlags);
          }
      }
      updateLastMouseLocation(point);
      return handled;
  }

  virtual bool onMouseWheel
    (
      double angleDeltaX,
      double angleDeltaY
     )
  {
      bool handled=false;
      for (auto&c: childReceivers_)
      {
          if (!handled)
          {
              handled=c->onMouseWheel(angleDeltaX, angleDeltaY);
          }
      }
      return handled;
  }

  virtual void onMouseLeavesViewer()
  {
      for (auto&c: childReceivers_)
      {
          c->onMouseLeavesViewer();
      }
  }

};




template<class Viewer>
class ViewWidgetAction
  : public InputReceiver<Viewer>
{

public:
  typedef std::shared_ptr<ViewWidgetAction> Ptr;

    boost::signals2::signal<void(bool)> actionIsFinished;
    boost::signals2::signal<void(const QString&)> userPrompt;

private:
  Ptr childAction_;

protected:
  virtual void finishAction(bool accepted=true)
  {
      actionIsFinished(accepted); //finished_=true;
  }

  virtual void launchChildAction(Ptr childAction)
  {
      if (childAction_)
          childAction_.reset();

      childAction_=childAction;
      auto cPtr = childAction.get();

      InputReceiver<Viewer>::registerChildReceiver( cPtr );

      childAction_->actionIsFinished.connect(
          [this,cPtr](bool)
          {
              InputReceiver<Viewer>::removeChildReceiver(
                  cPtr );

              // might have been reset by some other signal handler already...
              if (childAction_.get()==cPtr)
              {
                childAction_.reset();
              }
          });

      childAction_->start();
  }

  template<class A>
  bool isRunning() const
  {
      return bool(std::dynamic_pointer_cast<A>(childAction_));
  }

  template<class A>
  A* runningAction() const
  {
      return std::dynamic_pointer_cast<A>(childAction_).get();
  }

public:
  ViewWidgetAction(Viewer& viewer)
      : InputReceiver<Viewer>(viewer)
  {}

  ViewWidgetAction(Viewer& viewer, const QPoint& p)
      : InputReceiver<Viewer>(viewer, p)
  {}

  virtual void start() =0;
};




class OCCViewWidgetRotation : public ViewWidgetAction<QoccViewWidget>
{
public:
  OCCViewWidgetRotation(QoccViewWidget &viewWidget, const QPoint point);

  void start() override;

  bool onMouseMove
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

  void start() override;

  bool onMouseMove
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

  void start() override;

  bool onMouseMove
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

  void start() override;

  bool onMouseMove
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

  void start() override;

  bool onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
};



#endif // VIEWWIDGETACTION_H
