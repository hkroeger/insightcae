#ifndef INPUTRECEIVER_H
#define INPUTRECEIVER_H

#include "base/exception.h"
#include "boost/signals2.hpp"

#include <set>

#include <QWidget>


template<class Viewer>
class InputReceiver
{
public:
    struct Deleter
    {
        void operator()(InputReceiver* ir)
        {
            ir->aboutToBeDestroyed();
            delete ir;
        }
    };

    typedef Viewer Viewer_type;

    typedef std::unique_ptr<InputReceiver, Deleter> Ptr;

    boost::signals2::signal<void()> aboutToBeDestroyed;
    boost::signals2::signal<void(const QString&)> userPrompt, descriptionChanged;


    enum EventType
    {
        Begin, Intermediate, Final
    };

private:

    mutable bool cleanupDone_ = false;
    Viewer& viewer_;

    std::set<InputReceiver*> childReceivers_;

    void prepareCleanup()
    {
        aboutToBeDestroyed.connect([this](){cleanupDone_=true;});
    }

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
    {
        prepareCleanup();
    }

    InputReceiver(Viewer& viewer, const QPoint& p, bool captureAllInput)
        : viewer_(viewer),
        lastMouseLocation_(new QPoint(p)),
        capturesAllInput_(captureAllInput)
    {
        prepareCleanup();
    }

    virtual ~InputReceiver()
    {
        if (!cleanupDone_)
        {
            insight::Warning("internal error in InputReveiver: cleanup not done!");
        }
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


    virtual bool onDoubleClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
    {
        return
            toFirstChildAction(
                &InputReceiver::onDoubleClick,
                btn, nFlags, point )
            ||
            capturesAllInput_;
    }

    virtual bool onMouseClick  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point )
    {
        return
            toFirstChildAction(
                &InputReceiver::onMouseClick,
                btn, nFlags, point)
            ||
            capturesAllInput_;

    }

    /**
   * @brief onMouseDrag
   * mouse move and button pressed at the same time
   * @param btn
   * @param nFlags
   * @param point
   * @param eventType
   * whether this call is the first in dragging, some update or the final call.
   * @return
   */
    virtual bool onMouseDrag  (
        Qt::MouseButtons btn,
        Qt::KeyboardModifiers nFlags,
        const QPoint point,
        EventType eventType )
    {
        updateLastMouseLocation(point);

        return
            toFirstChildAction(
                &InputReceiver::onMouseDrag,
                btn, nFlags, point, eventType)
            ||
            capturesAllInput_;
    }

    /**
   * @brief onMouseMove
   * plain mouse move (no buttons pressed)
   * @param point
   * @param curFlags
   * @return
   */
    virtual bool onMouseMove(
        const QPoint point,
        Qt::KeyboardModifiers curFlags )
    {
        updateLastMouseLocation(point);

        return
            toFirstChildAction(
                &InputReceiver::onMouseMove,
                point, curFlags );
    }

    virtual bool onMouseWheel(
        double angleDeltaX,
        double angleDeltaY )
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

};




#endif // INPUTRECEIVER_H
