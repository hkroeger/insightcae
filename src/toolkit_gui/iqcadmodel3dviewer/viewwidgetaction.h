#ifndef VIEWWIDGETACTION_H
#define VIEWWIDGETACTION_H

#include "toolkit_gui_export.h"


#include "cadtypes.h"
#include "feature.h"
#include "boost/signals2.hpp"


#include <memory>

#include <QWidget>
#include <QRubberBand>

#include <QMetaObject>
#include <QApplication>
#include <qnamespace.h>
#include <typeindex>

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
  boost::signals2::signal<void(const QString&)> userPrompt;

private:
  Viewer& viewer_;

  std::set<InputReceiver*> childReceivers_;

protected:
  bool capturesAllInput_;

protected:
  /**
   * @brief currentPoint_
   * Track mouse location for use in key events.
   * Attention: invalid until first mouse event received!
   */
  std::unique_ptr<QPoint> lastMouseLocation_;

public:
  InputReceiver(Viewer& viewer, bool captureAllInput)
    : viewer_(viewer), capturesAllInput_(captureAllInput)
  {}

  InputReceiver(Viewer& viewer, const QPoint& p, bool captureAllInput)
  : viewer_(viewer),
    lastMouseLocation_(new QPoint(p)),
    capturesAllInput_(captureAllInput)
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
              if (childReceivers_.count(recv))
                childReceivers_.erase(recv);
          }
          );
      childReceivers_.insert(recv);
  }

  void removeChildReceiver(InputReceiver* recv)
  {
      childReceivers_.erase(recv);
  }

  bool hasChildReceivers() const
  {
      return childReceivers_.size()>0;
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

  template<class Function, typename... Args>
  bool toFirstChildAction(Function f, Args&&... args)
  {
      for (auto&c: childReceivers_)
      {
          if (
              std::bind(f, c, std::forward<Args>(args)...)()
              ) return true;
      }
      return false;
   }

  template<class Function, typename... Args>
  bool toAllChildActions(Function f, Args&&... args)
  {
      bool handled=false;
      for (auto&c: childReceivers_)
      {
          handled=
            handled
                    ||
            std::bind(f, c, std::forward<Args>(args)...)();
      }
      return handled;
  }

  template<class Function, typename... Args>
  void forAllChildActions(Function f, Args&&... args)
  {
      for (auto&c: childReceivers_)
      {
          std::bind(f, c, std::forward<Args>(args)...)();
      }
  }

  virtual bool onLeftButtonDoubleClick  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return
          toFirstChildAction(
                 &InputReceiver::onLeftButtonDoubleClick,
                   nFlags, point )
           ||
             capturesAllInput_;
  }

  virtual bool onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point, bool afterDoubleClick )
  {
      return
          toFirstChildAction(
              &InputReceiver::onLeftButtonDown,
                        nFlags, point, afterDoubleClick)
          ||
             capturesAllInput_;

  }

  virtual bool onMiddleButtonDown( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return
          toFirstChildAction(
              &InputReceiver::onMiddleButtonDown,
                        nFlags, point )
          ||
          capturesAllInput_;
  }

  virtual bool onRightButtonDown ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return
          toFirstChildAction(
              &InputReceiver::onRightButtonDown,
                        nFlags, point )
          ||
          capturesAllInput_;
  }

  virtual bool onLeftButtonUp    ( Qt::KeyboardModifiers nFlags, const QPoint point, bool afterDoubleClick )
  {
      return
          toFirstChildAction(
              &InputReceiver::onLeftButtonUp,
                        nFlags, point, afterDoubleClick )
          ||
          capturesAllInput_;
  }

  virtual bool onMiddleButtonUp  ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return
          toFirstChildAction(
              &InputReceiver::onMiddleButtonUp,
                        nFlags, point )
          ||
          capturesAllInput_;
  }

  virtual bool onRightButtonUp   ( Qt::KeyboardModifiers nFlags, const QPoint point )
  {
      return
          toFirstChildAction(
              &InputReceiver::onRightButtonUp,
                        nFlags, point )
          ||
          capturesAllInput_;
  }

  virtual bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
  {
      return
          toFirstChildAction(
              &InputReceiver::onKeyPress,
                        modifiers, key )
          ||
          capturesAllInput_;
  }

  virtual bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
  {
      return
          toFirstChildAction(
              &InputReceiver::onKeyRelease,
              modifiers, key )
          ||
          capturesAllInput_;
  }

  virtual bool onMouseMove
    (
     Qt::MouseButtons buttons,
     const QPoint point,
     Qt::KeyboardModifiers curFlags
     )
  {
      bool handled=
          toFirstChildAction(
              &InputReceiver::onMouseMove,
              buttons, point, curFlags );

      updateLastMouseLocation(point);

      return handled;
  }

  virtual bool onMouseWheel
    (
      double angleDeltaX,
      double angleDeltaY
     )
  {
      return
          toFirstChildAction(
            &InputReceiver::onMouseWheel,
            angleDeltaX, angleDeltaY );
  }

  virtual void onMouseLeavesViewer()
  {
      forAllChildActions(
        &InputReceiver::onMouseLeavesViewer);
  }

};




template<class Viewer>
class ViewWidgetAction;



/**
 * @brief The ViewWidgetActionHost class
 * an input receiver that may run an action.
 * If not special action is launched
 */
template<class Viewer>
class ViewWidgetActionHost
    : public InputReceiver<Viewer>
{

public:
    typedef
        std::shared_ptr<ViewWidgetAction<Viewer> >
        ViewWidgetActionPtr;

private:
    ViewWidgetActionPtr currentAction_;
    mutable boost::optional<std::type_index> defaultActionType_;

protected:
    /**
     * @brief setupDefaultAction
     * @return
     * empty ptr, if no default action
     */
    virtual ViewWidgetActionPtr setupDefaultAction()
    {
        return nullptr;
    }

    void prepareDestruction()
    {
        this->aboutToBeDestroyed.connect(
            [this]() { currentAction_.reset(); } );
    }

public:
    ViewWidgetActionHost(Viewer& v, bool captureAllInput)
        : InputReceiver<Viewer>(v, captureAllInput)
    {
        prepareDestruction();
    }

    ViewWidgetActionHost(ViewWidgetActionHost& parent, bool captureAllInput)
        : InputReceiver<Viewer>(parent.viewer(), captureAllInput)
    {
        prepareDestruction();
    }

    ViewWidgetActionHost(ViewWidgetActionHost& parent, const QPoint& p, bool captureAllInput)
        : InputReceiver<Viewer>(parent.viewer(), p, captureAllInput)
    {
        prepareDestruction();
    }


    void setDefaultAction()
    {
        if (ViewWidgetActionPtr da = setupDefaultAction())
        {
            defaultActionType_ = typeid(*da);
            launchAction(da);
        }
    }

    bool isDefaultAction() const
    {
        if (defaultActionType_.is_initialized())
        {
            if (currentAction_)
            {
                return
                    std::type_index(typeid(*currentAction_))
                       == *defaultActionType_;
            }
        }
        return false;
    }

    virtual bool launchAction(
        ViewWidgetActionPtr childAction,
        bool force=true )
    {
        if (currentAction_ && force)
            currentAction_.reset();

        if (!currentAction_)
        {
            currentAction_=childAction;
            auto cPtr = childAction.get();

            InputReceiver<Viewer>::registerChildReceiver( cPtr );

            currentAction_->connectActionIsFinished(
                [this,cPtr](bool)
                {
                    InputReceiver<Viewer>::removeChildReceiver(
                        cPtr );

                    // might have been reset by some other signal handler already...
                    if (currentAction_.get()==cPtr)
                    {
                        currentAction_.reset();
                    }

                    setDefaultAction();
                });

            currentAction_->userPrompt.connect(
                this->userPrompt );


            currentAction_->start();

            return true;
        }

        return false;
    }

    template<class A = ViewWidgetAction<Viewer> >
    bool isRunning() const
    {
        return bool(std::dynamic_pointer_cast<A>(currentAction_));
    }

    template<class A = ViewWidgetAction<Viewer> >
    A* runningAction() const
    {
        return std::dynamic_pointer_cast<A>(currentAction_).get();
    }

    void cancelCurrentAction()
    {
        if (isRunning() && !isDefaultAction())
        {
            currentAction_.reset();
            setDefaultAction();
        }
    }

    ViewWidgetActionHost<Viewer>* topmostActionHost() const
    {
        if (auto *a=this->runningAction())
            return a->topmostActionHost();
        else
            return this;
    }
};




template<class Viewer>
class ViewWidgetAction
  : public ViewWidgetActionHost<Viewer>
{

public:
    /**
     * @brief connectActionIsFinished
     * queue into event loop, don't execute directly
     * since object might delete itself
     * @param f
     */
    void connectActionIsFinished(std::function<void(bool)> f)
    {
        actionIsFinished.connect([f](bool success){
            QMetaObject::invokeMethod(
                qApp,
                std::bind(f, success),
                Qt::QueuedConnection
                );
        });
    }

private:
    boost::signals2::signal<void(bool)> actionIsFinished;


protected:
  virtual void finishAction(bool accepted=true)
  {
      actionIsFinished(accepted); //finished_=true;
  }

public:
  using ViewWidgetActionHost<Viewer>::ViewWidgetActionHost;

  ViewWidgetAction(ViewWidgetActionHost<Viewer>& parent, bool captureAllInput=true)
    : ViewWidgetActionHost<Viewer>(parent, captureAllInput)
  {}

  bool onKeyPress(Qt::KeyboardModifiers modifiers, int key) override
  {
      if (!this->toFirstChildAction(
          &InputReceiver<Viewer>::onKeyPress,
              modifiers, key))
      {
          if (key == Qt::Key_Escape)
          {
              finishAction();
              return true;
          }
      }
      return this->capturesAllInput_;
  }

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

  bool onLeftButtonUp( Qt::KeyboardModifiers nFlags, const QPoint point,
                      bool lastClickWasDoubleClick ) override;
};



#endif // VIEWWIDGETACTION_H
