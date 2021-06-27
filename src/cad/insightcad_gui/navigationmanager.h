#ifndef NAVIGATIONMANAGER_H
#define NAVIGATIONMANAGER_H




#include "viewwidgetaction.h"




class NavigationManager : public InputReceiver
{
  ViewWidgetActionPtr& currentAction_;

protected:
  bool actionInProgress();

  template<class T>
  std::shared_ptr<T> currentAction()
  {
    return std::dynamic_pointer_cast<T>(currentAction_);
  }

  void setCurrentAction(ViewWidgetActionPtr newAct);
  void stopCurrentAction();

  QoccViewWidget* viewWidget_;
  Handle_V3d_View& view_;

public:
  NavigationManager(ViewWidgetActionPtr& currentAction, QoccViewWidget* viewWidget, Handle_V3d_View& view);

  void onMouseWheel
      (
        double angleDeltaX,
        double angleDeltaY
       ) override;

  void onLeftButtonDown  ( Qt::KeyboardModifiers nFlags, const QPoint point ) override;
  void onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;
};




class TouchpadNavigationManager : public NavigationManager
{

public:
  TouchpadNavigationManager(ViewWidgetActionPtr& currentAction, QoccViewWidget* viewWidget, Handle_V3d_View& view);
  void onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override;
  void onKeyRelease ( Qt::KeyboardModifiers modifiers, int key) override;
};




#endif // NAVIGATIONMANAGER_H
