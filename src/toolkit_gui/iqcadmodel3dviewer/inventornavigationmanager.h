#ifndef INVENTORNAVIGATIONMANAGER_H
#define INVENTORNAVIGATIONMANAGER_H

#include "navigationmanager.h"



template<class Viewer, class Panning, class Rotation>
class InventorNavigationManager
    : public NavigationManager<Viewer>
{

    bool possibleRotation_;

public:
    declareType("Inventor");

    InventorNavigationManager(
        Viewer& viewer )
        : NavigationManager<Viewer>(viewer)
    {}

    bool onKeyPress ( Qt::KeyboardModifiers modifiers, int key ) override
    {
        return NavigationManager<Viewer>::onKeyPress(modifiers, key);
    }

    bool onKeyRelease ( Qt::KeyboardModifiers modifiers, int key) override
    {
        return NavigationManager<Viewer>::onKeyRelease(modifiers, key);
    }

    bool onMouseDrag(
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        typename InputReceiver<Viewer>::EventType eventType ) override
    {
        if (eventType==InputReceiver<Viewer>::EventType::Final)
        {
            this->stopCurrentAction();
        }
        else
        {
            if (btn==Qt::LeftButton)
            {
                // rotate
                if (!this->actionInProgress() && this->hasLastMouseLocation())
                {
                    this->setCurrentAction( make_viewWidgetAction<Rotation>(
                        this->viewer(), this->lastMouseLocation()) );
                    return true;
                }
            }
            else if (btn==Qt::MiddleButton)
            {
                // pan
                if (!this->actionInProgress() && this->hasLastMouseLocation())
                {
                    this->setCurrentAction( make_viewWidgetAction<Panning>(
                        this->viewer(), this->lastMouseLocation()) );
                    return true;
                }
            }
        }

        return InputReceiver<Viewer>::onMouseDrag(btn, nFlags, point, eventType);
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
            return InputReceiver<Viewer>::onMouseWheel(
                angleDeltaX, angleDeltaY );
    }

};


#endif // INVENTORNAVIGATIONMANAGER_H
