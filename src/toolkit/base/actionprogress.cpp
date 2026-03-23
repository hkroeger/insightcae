#include "actionprogress.h"

#include "base/exception.h"
#include "base/progressdisplayer.h"
#include "base/boost_include.h"
#include <iostream>

namespace insight {


std::string ActionProgress::actionPath() const
{
    std::string p = name_;
    ElementPath prefix;

    if (auto *pa=boost::get<ActionProgressPtr>(&parent_))
        prefix=(*pa)->actionPath();

    if (!prefix.empty())
        p=prefix/p;

    return p;
}




ProgressDisplayer &ActionProgress::parentDisplayer()
{
    if (auto *pa=boost::get<ActionProgressPtr>(&parent_))
        return (*pa)->parentDisplayer();
    else
        return *boost::get<ProgressDisplayer*>(parent_);
}




ActionProgress* ActionProgress::getChildAction(
    const std::string& pathstr )
{
    ElementPath ep(pathstr);

    insight::assertion(
        ep.size(), "empty path given");

    auto firstElem=ep.front();
    ep.erase(ep.begin());

    auto i=std::find_if(
        children_.begin(), children_.end(),
        [firstElem](ActionProgress* ap)
        { return ap->name()==firstElem; } );

    if (i==children_.end())
        return nullptr;

    if (ep.size())
        return (*i)->getChildAction(ep);
    else
        return *i;
}



void ActionProgress::startAction()
{
    if (auto *pa=boost::get<ActionProgressPtr>(&parent_))
        (*pa)->children_.insert(this);
    else
    {
        auto& pd = *boost::get<ProgressDisplayer*>(parent_);
        std::lock_guard<std::mutex> lock(pd.childActionsMutex_);
        pd.childActions_.insert(this);
    }

    stepTo(0); // trigger creation of progress bar // virtual functions...
    message(str(boost::format("Starting %d...")%name_));
}



ActionProgress::ActionProgress(
    ProgressDisplayer *parent,
    std::string name,
    double nSteps )
 : parent_(parent),
    name_(name),
    maxi_(nSteps)
{}



ActionProgress::ActionProgress(
    ActionProgressPtr parentAction,
    std::string name,
    double nSteps )
 : parent_(parentAction),
   name_(name),
    maxi_(nSteps)
{
    startAction();
}




ActionProgress::~ActionProgress()
{
    if (!skipFinishOnDestroy_)
        parentDisplayer().finishActionProgress(actionPath());
    if (auto *pa=boost::get<ActionProgressPtr>(&parent_))
        (*pa)->children_.erase(this);
    else if (!skipFinishOnDestroy_)
    {
        // When the displayer is being destroyed it sets skipFinishOnDestroy_
        // before its members are torn down; skip the erase to avoid
        // accessing childActions_ on a dying ProgressDisplayer.
        auto& pd = *boost::get<ProgressDisplayer*>(parent_);
        std::lock_guard<std::mutex> lock(pd.childActionsMutex_);
        pd.childActions_.erase(this);
    }
}


bool ActionProgress::stopIsDemanded() const
{
    return parentDisplayer().stopIsDemanded();
}


ActionProgressPtr
ActionProgress::forkNewAction(
    double nSteps, const std::string& name )
{
    return ActionProgressPtr(
        new ActionProgress(shared_from_this(), name, nSteps));
}

const std::string &ActionProgress::name() const
{
    return name_;
}


void ActionProgress::stepUp(double steps)
{
    stepTo(ci_+steps);
}

void ActionProgress::stepUp(
    const std::string& msg, double steps )
{
    stepTo(ci_+steps);
    message(msg);
}

void ActionProgress::stepTo(double i)
{
    ci_=std::min(maxi_,i);
    insight::dbg()<<actionPath()<<": progress "<<ci_<<"/"<<maxi_<<std::endl;
    parentDisplayer().setActionProgressValue(actionPath(), ci_/maxi_);
    if (ci_ >= maxi_)
    {
        insight::dbg()<<actionPath()<<": finish progress"<<std::endl;
        parentDisplayer().finishActionProgress(actionPath());
    }
}

void ActionProgress::completed()
{
    if (ci_<maxi_)
    {
        stepTo(maxi_);
    }
}

void ActionProgress::operator++()
{
    stepUp();
}

void ActionProgress::operator+=(double n)
{
    stepUp(n);
}

void ActionProgress::message(const std::string &message)
{
    insight::dbg()<<actionPath()<<": "<<message<<std::endl;
    parentDisplayer().setMessageText(actionPath(), message);
}



void ActionProgress::setNSteps(int nSteps)
{
    maxi_=nSteps+1;
}

bool ActionProgress::isStoppable()
{
    return stopSignal_.num_slots();
}


ActionProgress::StopSignal&
ActionProgress::stopSignal()
{
    return stopSignal_;
}




void ActionProgress::triggerStop()
{
    stopSignal_();
}


} // namespace insight
