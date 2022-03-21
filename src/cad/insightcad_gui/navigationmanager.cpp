#include "navigationmanager.h"

#include "qoccviewwidget.h"
#include "base/exception.h"


bool NavigationManager::actionInProgress()
{
  return bool(currentAction_);
}




void NavigationManager::setCurrentAction(ViewWidgetActionPtr newAct)
{
  currentAction_.reset(); // delete first
  currentAction_ = newAct;
}




void NavigationManager::stopCurrentAction()
{
  currentAction_.reset();
}




NavigationManager::NavigationManager(
    ViewWidgetActionPtr &currentAction,
    QoccViewWidget *viewWidget,
    Handle_V3d_View &view )
  : currentAction_(currentAction),
    viewWidget_(viewWidget),
    view_(view)
{}




void NavigationManager::onMouseWheel(double /*angleDeltaX*/, double angleDeltaY)
{
  if (angleDeltaY>0)
  {
    view_->SetScale( view_->Scale() * 1.1 ); // +10%
  }
  else if (angleDeltaY<0)
  {
    view_->SetScale( view_->Scale() / 1.1 ); // -10%
  }
}




void NavigationManager::onLeftButtonDown(Qt::KeyboardModifiers modifiers, const QPoint /*point*/)
{
  AIS_StatusOfPick picked = AIS_SOP_NothingSelected;

  if ( modifiers&Qt::ControlModifier )
  {
    picked = viewWidget_->getContext()->ShiftSelect(
#if (OCC_VERSION_MAJOR>=7)
                  true
#endif
          );
  }
  else
  {
    picked = viewWidget_->getContext()->Select(
#if (OCC_VERSION_MAJOR>=7)
                  true
#endif
          );
  }

  if ( picked != AIS_SOP_NothingSelected )
  {
    viewWidget_->graphicalSelectionChanged(viewWidget_->getSelectedItem(), viewWidget_);
  }
}




void NavigationManager::onKeyPress(Qt::KeyboardModifiers modifiers, int key)
{
  if ( key==Qt::Key_Plus && (modifiers&Qt::ControlModifier) )
  {
    view_->SetScale( view_->Scale() * 1.1 ); // +10%
  }
  else if (key==Qt::Key_Minus && (modifiers&Qt::ControlModifier))
  {
    view_->SetScale( view_->Scale() / 1.1 ); // -10%
  }
}









TouchpadNavigationManager::TouchpadNavigationManager(
    ViewWidgetActionPtr &currentAction,
    QoccViewWidget *viewWidget,
    Handle_V3d_View &view )
  : NavigationManager(currentAction, viewWidget, view)
{}




void TouchpadNavigationManager::onKeyPress ( Qt::KeyboardModifiers modifiers, int key )
{
  if (modifiers & Qt::ShiftModifier)
  {
    if (!actionInProgress())
      setCurrentAction( std::make_shared<ViewWidgetPanning>(view_, lastMouseLocation()) );
  }
  else if (modifiers & Qt::AltModifier)
  {
    if (!actionInProgress())
      setCurrentAction( std::make_shared<ViewWidgetRotation>(view_, lastMouseLocation()) );
  }
  else if (key==Qt::Key_PageUp)
  {
    view_->SetScale( view_->Scale() * 1.1 ); // +10%
  }
  else if (key==Qt::Key_PageDown)
  {
    view_->SetScale( view_->Scale() / 1.1 ); // -10%
  }

  NavigationManager::onKeyPress(modifiers, key);
}




void TouchpadNavigationManager::onKeyRelease ( Qt::KeyboardModifiers modifiers, int key )
{
  if ( !(modifiers & Qt::ShiftModifier)
       && currentAction<ViewWidgetPanning>() )
  {
    stopCurrentAction();
  }
  else if ( !(modifiers & Qt::AltModifier)
       && currentAction<ViewWidgetRotation>() )
  {
    stopCurrentAction();
  }

  NavigationManager::onKeyRelease(modifiers, key);
}

