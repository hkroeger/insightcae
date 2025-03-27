#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H

#include <QMouseEvent>


#include "base/exception.h"
#include "viewwidgetaction.h"
#include <qnamespace.h>





template<class Viewer>
class NavigationManager
    : public InputReceiver<Viewer>
{
public:
  typedef
        std::unique_ptr<
            NavigationManager,
            typename InputReceiver<Viewer>::Deleter
        > Ptr;

private:
  typename InputReceiver<Viewer>::Ptr currentAction_;

protected:
  bool actionInProgress()
  {
    return bool(currentAction_);
  }

  template<class T>
  T* currentAction()
  {
    return dynamic_cast<T*>(currentAction_.get());
  }

  void setCurrentAction(typename InputReceiver<Viewer>::Ptr newAct)
  {
    currentAction_.reset(); // delete first
    currentAction_ = std::move(newAct);
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
  declareType("Navigation");

  NavigationManager( Viewer& viewer )
  : InputReceiver<Viewer>(viewer, false)
  {}



  // bool onLeftButtonDown( Qt::KeyboardModifiers modifiers, const QPoint point, bool afterDoubleClick ) override
  // {
  //   if ( this->viewer().pickAtCursor( modifiers&Qt::ControlModifier ) )
  //   {
  //     this->viewer().emitGraphicalSelectionChanged();
  //     return true;
  //   }
  //   return InputReceiver<Viewer>::onLeftButtonDown(modifiers, point, afterDoubleClick);
  // }

  // bool onKeyPress( Qt::KeyboardModifiers modifiers, int key ) override
  // {
  //   if (key == Qt::Key_Escape)
  //   {
  //     currentAction_.reset();
  //   }
  //   if ( key==Qt::Key_Plus && (modifiers&Qt::ControlModifier) )
  //   {
  //     scaleUp();
  //     return true;
  //   }
  //   else if (key==Qt::Key_Minus && (modifiers&Qt::ControlModifier))
  //   {
  //     scaleDown();
  //     return true;
  //   }
  //   return InputReceiver<Viewer>::onKeyPress(modifiers, key);
  // }

};




template<class Viewer>
class MouseKeyboardInputAdapter
{
    NavigationManager<Viewer>& target_;
public:
    MouseKeyboardInputAdapter(
        NavigationManager<Viewer>& target
    )
        : target_(target)
    {}
};







#endif // NAVIGATIONMANAGER_H
