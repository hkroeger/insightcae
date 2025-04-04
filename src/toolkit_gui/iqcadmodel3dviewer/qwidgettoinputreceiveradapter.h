#ifndef QWIDGETTOINPUTRECEIVERADAPTER_H
#define QWIDGETTOINPUTRECEIVERADAPTER_H

#include "navigationmanager.h"
#include "inventornavigationmanager.h"


template<class Viewer, class Widget>
class QWidgetToInputReceiverAdapter
    : public Widget,
      public ViewWidgetActionHost<Viewer>
{

protected:
    typename NavigationManager<Viewer>::Ptr navigationManager_;
    mutable boost::optional<Qt::MouseButtons> currentDrag_;
    mutable Qt::MouseButtons lastButtonState_;
    bool doubleClickAppeared_;

public:
    QWidgetToInputReceiverAdapter(Viewer& v, QWidget *parent)
        : Widget(parent),
        ViewWidgetActionHost<Viewer>(v, false),
        doubleClickAppeared_(false)
    {}

    virtual void resetNavigationManager(typename NavigationManager<Viewer>::Ptr&& nm)
    {
        navigationManager_.reset();
        navigationManager_=std::move(nm);
    }

    const NavigationManager<Viewer>& currentNavigationManager() const
    {
        return *navigationManager_;
    }

protected:

    template<class Function, typename... Args>
    void call(Function f, Args&&... args)
    {
        bool ret =
            std::bind(f, this, std::forward<Args>(args)...)();
        if (!ret && navigationManager_)
            std::bind(f, navigationManager_.get(), std::forward<Args>(args)...)();
    }


    void mouseDoubleClickEvent(QMouseEvent* e) override
    {
        insight::dbg(3)<<">>> mouse double click " << e->buttons() <<std::endl;

        Widget::mouseDoubleClickEvent(e);

        doubleClickAppeared_=true;
        call(
            &InputReceiver<Viewer>::onDoubleClick,
            e->buttons(), e->modifiers(), e->pos()
            );
    }


    void mousePressEvent(QMouseEvent* e) override
    {
        insight::dbg(3)<<">>> mouse press " << e->buttons() <<std::endl;

        Widget::mousePressEvent(e);

        lastButtonState_=e->buttons();
    }


    void mouseReleaseEvent(QMouseEvent* e) override
    {
        insight::dbg(3)<<">>> mouse release " << e->buttons() <<std::endl;

        Widget::mouseReleaseEvent(e);

        auto causingButton = lastButtonState_ ^ e->buttons();

        if (!doubleClickAppeared_) // ignore second mouse press/release pair after double click
        {

            if (currentDrag_)
            {
                // btn released => dragging ends
                call(
                    &InputReceiver<Viewer>::onMouseDrag,
                    causingButton, e->modifiers(), e->pos(),
                    InputReceiver<Viewer>::EventType::Final
                    );
                currentDrag_ = boost::none;
            }
            else
            {
                // mouse click
                call(
                    &InputReceiver<Viewer>::onMouseClick,
                    causingButton, e->modifiers(), e->pos() );
            }

        }

        lastButtonState_=e->buttons();
        doubleClickAppeared_=false;
    }


    void mouseMoveEvent(QMouseEvent* e) override
    {
        Widget::mouseMoveEvent(e);

        auto btn=e->buttons();

        if (btn && !currentDrag_)
        {
            // start
            call(
                &InputReceiver<Viewer>::onMouseDrag,
                btn, e->modifiers(), e->pos(),
                InputReceiver<Viewer>::EventType::Begin
                );
            currentDrag_ = btn;

        }
        else if (!btn && currentDrag_)
        {
            // stop
            call(
                &InputReceiver<Viewer>::onMouseDrag,
                btn, e->modifiers(), e->pos(),
                InputReceiver<Viewer>::EventType::Final
                );
            currentDrag_ = boost::none;
        }
        else if (!btn && !currentDrag_)
        {
            // plain move
            call(
                &InputReceiver<Viewer>::onMouseMove,
                e->pos(), e->modifiers()
                );
        }
        else // btn && currentDrag
        {
            if (btn != *currentDrag_)
            {
                // changed btn
                call(
                    &InputReceiver<Viewer>::onMouseDrag,
                    *currentDrag_, e->modifiers(), e->pos(),
                    InputReceiver<Viewer>::EventType::Final
                    );
                call(
                    &InputReceiver<Viewer>::onMouseDrag,
                    btn, e->modifiers(), e->pos(),
                    InputReceiver<Viewer>::EventType::Begin
                    );
                currentDrag_ = btn;
            }
            else
            {
                // continue with btn unchanged
                call(
                    &InputReceiver<Viewer>::onMouseDrag,
                    btn, e->modifiers(), e->pos(),
                    InputReceiver<Viewer>::EventType::Intermediate
                    );
            }
        }
    }


    void wheelEvent(QWheelEvent* e) override
    {
        insight::dbg(3)<<"wheel"<<std::endl;

        Widget::wheelEvent(e);

        call(
            &InputReceiver<Viewer>::onMouseWheel,
            e->angleDelta().x(), e->angleDelta().y()
            );
    }


    void keyPressEvent(QKeyEvent* e) override
    {
        insight::dbg(3)<<"key press"<<std::endl;

        Widget::keyPressEvent(e);

        call(
            &InputReceiver<Viewer>::onKeyPress,
            e->modifiers(), e->key()
            );
    }


    void keyReleaseEvent(QKeyEvent* e) override
    {
        insight::dbg(3)<<"key release"<<std::endl;

        Widget::keyReleaseEvent(e);

        call(
            &InputReceiver<Viewer>::onKeyRelease,
            e->modifiers(), e->key()
            );
    }

};


#endif // QWIDGETTOINPUTRECEIVERADAPTER_H
