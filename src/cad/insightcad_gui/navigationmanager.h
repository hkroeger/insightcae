#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H




#include "viewwidgetaction.h"


template<class Viewer>
class NavigationManager : public InputReceiver<Viewer>
{
public:
  typedef std::shared_ptr<NavigationManager> Ptr;

private:
  typename InputReceiver<Viewer>::Ptr& currentAction_;

protected:
  bool actionInProgress()
  {
    return bool(currentAction_);
  }

  template<class T>
  std::shared_ptr<T> currentAction()
  {
    return std::dynamic_pointer_cast<T>(currentAction_);
  }

  void setCurrentAction(typename InputReceiver<Viewer>::Ptr newAct)
  {
    currentAction_.reset(); // delete first
    currentAction_ = newAct;
  }

  void stopCurrentAction()
  {
    currentAction_.reset();
  }

  void scaleUp()
  {
      this->viewer().setScale(
                    this->viewer().getScale() * 1.1 ); // +10%
  }

  void scaleDown()
  {
      this->viewer().setScale(
                    this->viewer().getScale() / 1.1 ); // -10%
  }

public:
  NavigationManager(
          typename InputReceiver<Viewer>::Ptr& currentAction,
          Viewer& viewer )
  : InputReceiver<Viewer>(viewer),
    currentAction_(currentAction)
  {}

  void onMouseWheel
      (
        double angleDeltaX,
        double angleDeltaY
       ) override
  {
    if (angleDeltaY>0)
    {
        scaleUp();
    }
    else if (angleDeltaY<0)
    {
        scaleDown();
    }
  }

  void onLeftButtonDown  ( Qt::KeyboardModifiers modifiers, const QPoint point ) override
  {
    if ( this->viewer().pickAtCursor( modifiers&Qt::ControlModifier ) )
    {
      this->viewer().emitGraphicalSelectionChanged();
    }
  }

  void onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override
  {
    if ( key==Qt::Key_Plus && (modifiers&Qt::ControlModifier) )
    {
        scaleUp();
    }
    else if (key==Qt::Key_Minus && (modifiers&Qt::ControlModifier))
    {
        scaleDown();
    }
  }

};



template<class Viewer, class Panning, class Rotation>
class TouchpadNavigationManager
        : public NavigationManager<Viewer>
{

public:
  TouchpadNavigationManager(
          typename InputReceiver<Viewer>::Ptr& currentAction,
          Viewer& viewer )
      : NavigationManager<Viewer>(currentAction, viewer)
  {}

  void onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override
  {
      if (modifiers & Qt::ShiftModifier)
      {
        if (!this->actionInProgress() && this->hasLastMouseLocation() )
          this->setCurrentAction( std::make_shared<Panning>(
                                this->viewer(), this->lastMouseLocation()) );
      }
      else if (modifiers & Qt::AltModifier)
      {
        if (!this->actionInProgress() && this->hasLastMouseLocation())
          this->setCurrentAction( std::make_shared<Rotation>(
                                this->viewer(), this->lastMouseLocation()) );
      }
      else if (key==Qt::Key_PageUp)
      {
        this->scaleUp();
      }
      else if (key==Qt::Key_PageDown)
      {
        this->scaleDown();
      }

      NavigationManager<Viewer>::onKeyPress(modifiers, key);
  }

  void onKeyRelease ( Qt::KeyboardModifiers modifiers, int key) override
  {
    if ( !(modifiers & Qt::ShiftModifier)
         && this->template currentAction<Panning>() )
    {
      this->stopCurrentAction();
    }
    else if ( !(modifiers & Qt::AltModifier)
         && this->template currentAction<Rotation>() )
    {
      this->stopCurrentAction();
    }

    NavigationManager<Viewer>::onKeyRelease(modifiers, key);
  }

};




#endif // NAVIGATIONMANAGER_H
