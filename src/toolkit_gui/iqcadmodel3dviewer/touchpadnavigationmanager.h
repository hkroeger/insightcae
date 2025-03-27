#ifndef TOUCHPADNAVIGATIONMANAGER_H
#define TOUCHPADNAVIGATIONMANAGER_H

#include "navigationmanager.h"



template<class Viewer, class Panning, class Rotation>
class TouchpadNavigationManager
    : public NavigationManager<Viewer>
{

public:
    declareType("Touchpad");

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
                this->setCurrentAction( make_viewWidgetAction<Panning>(
                    this->viewer(), this->lastMouseLocation()) );
                return true;
            }
        }
        else if (modifiers & Qt::AltModifier)
        {
            if (!this->actionInProgress() && this->hasLastMouseLocation())
            {
                this->setCurrentAction( make_viewWidgetAction<Rotation>(
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

    bool onMouseWheel
        (
            double angleDeltaX,
            double angleDeltaY
            ) override
    {
        if (angleDeltaY>0)
        {
            this->scaleUp();
            return true;
        }
        else if (angleDeltaY<0)
        {
            this->scaleDown();
            return true;
        }
        else
            InputReceiver<Viewer>::onMouseWheel(
                angleDeltaX, angleDeltaY );
    }
};



#endif // TOUCHPADNAVIGATIONMANAGER_H
