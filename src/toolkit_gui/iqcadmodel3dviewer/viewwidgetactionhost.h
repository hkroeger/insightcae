#ifndef VIEWWIDGETACTIONHOST_H
#define VIEWWIDGETACTIONHOST_H


#include "iqcadmodel3dviewer/inputreceiver.h"


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
        std::unique_ptr<ViewWidgetAction<Viewer>, typename InputReceiver<Viewer>::Deleter >
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
            [this]() {
                currentAction_.reset();
            } );
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
            launchAction(std::move(da));
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
            auto cPtr = childAction.get();
            currentAction_=std::move(childAction);

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

            this->descriptionChanged(
                currentAction_->description());

            currentAction_->descriptionChanged.connect(
                this->descriptionChanged );

            currentAction_->start();

            return true;
        }

        return false;
    }

    template<class A = ViewWidgetAction<Viewer> >
    bool isRunning() const
    {
        return bool(dynamic_cast<const A*>(currentAction_.get()));
    }

    template<class A = ViewWidgetAction<Viewer> >
    A* runningAction() const
    {
        return dynamic_cast<const A*>(currentAction_.get());
    }

    void cancelCurrentAction(bool launchDefaultAction=true)
    {
        if (isRunning() && !isDefaultAction())
        {

            currentAction_.reset();

            if (launchDefaultAction)
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



#endif // VIEWWIDGETACTIONHOST_H
