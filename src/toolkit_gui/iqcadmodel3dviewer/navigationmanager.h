#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H




#include "viewwidgetaction.h"


template<class Viewer>
class NavigationManager
    : public InputReceiver<Viewer>
{
public:
  typedef std::shared_ptr<NavigationManager> Ptr;

private:
  typename InputReceiver<Viewer>::Ptr currentAction_;

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
    this->registerChildReceiver(currentAction_.get());
  }

  void stopCurrentAction()
  {
    this->removeChildReceiver(currentAction_.get());
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
  NavigationManager( Viewer& viewer )
  : InputReceiver<Viewer>(viewer)
  {}

  bool onMouseWheel
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
    return InputReceiver<Viewer>::onMouseWheel(angleDeltaX, angleDeltaY);
  }

  bool onLeftButtonDown( Qt::KeyboardModifiers modifiers, const QPoint point, bool afterDoubleClick ) override
  {
    if ( this->viewer().pickAtCursor( modifiers&Qt::ControlModifier ) )
    {
      this->viewer().emitGraphicalSelectionChanged();
      return true;
    }
    return InputReceiver<Viewer>::onLeftButtonDown(modifiers, point, afterDoubleClick);
  }

  bool onKeyPress( Qt::KeyboardModifiers modifiers, int key ) override
  {
    if (key == Qt::Key_Escape)
    {
      currentAction_.reset();
    }
    if ( key==Qt::Key_Plus && (modifiers&Qt::ControlModifier) )
    {
      scaleUp();
      return true;
    }
    else if (key==Qt::Key_Minus && (modifiers&Qt::ControlModifier))
    {
      scaleDown();
      return true;
    }
    return InputReceiver<Viewer>::onKeyPress(modifiers, key);
  }

};



template<class Viewer, class Panning, class Rotation>
class TouchpadNavigationManager
        : public NavigationManager<Viewer>
{

public:
  TouchpadNavigationManager(
          Viewer& viewer )
      : NavigationManager<Viewer>(viewer)
  {}

  bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override
  {
      if (modifiers & Qt::ShiftModifier)
      {
        if (!this->actionInProgress() && this->hasLastMouseLocation() )
        {
          this->setCurrentAction( std::make_shared<Panning>(
                                this->viewer(), this->lastMouseLocation()) );
            return true;
        }
      }
      else if (modifiers & Qt::AltModifier)
      {
        if (!this->actionInProgress() && this->hasLastMouseLocation())
        {
          this->setCurrentAction( std::make_shared<Rotation>(
                                this->viewer(), this->lastMouseLocation()) );
            return true;
        }
      }
      else if (key==Qt::Key_PageUp)
      {
        this->scaleUp();
        return true;
      }
      else if (key==Qt::Key_PageDown)
      {
        this->scaleDown();
        return true;
      }

      return NavigationManager<Viewer>::onKeyPress(modifiers, key);
  }

  bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key) override
  {
    if ( !(modifiers & Qt::ShiftModifier)
         && this->template currentAction<Panning>() )
    {
      this->stopCurrentAction();
        return true;
    }
    else if ( !(modifiers & Qt::AltModifier)
         && this->template currentAction<Rotation>() )
    {
      this->stopCurrentAction();
        return true;
    }

    return NavigationManager<Viewer>::onKeyRelease(modifiers, key);
  }

};




#endif // NAVIGATIONMANAGER_H
