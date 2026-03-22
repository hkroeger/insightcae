#include "progressdisplayer.h"
#include "base/exception.h"
#include "base/boost_include.h"
#include "base/actionprogress.h"
#include "base/elementpath.h"
#include "boost/algorithm/string/classification.hpp"

namespace insight {




ProgressState::ProgressState()
{}

ProgressState::ProgressState(
    double t,
    ProgressVariableList pvl,
    const std::string &message
    )
  : std::pair<double, ProgressVariableList>(t, pvl),
    logMessage_(message)
{}




ProgressDisplayer::ProgressDisplayer()
{}




ProgressDisplayer::~ProgressDisplayer()
{
    // Set the flag on every still-alive root action so that their destructors
    // neither call the pure-virtual finishActionProgress() nor try to erase
    // from childActions_ on this already-dying object.
    std::lock_guard<std::mutex> lock(childActionsMutex_);
    for (auto* ap : childActions_)
        ap->skipFinishOnDestroy_ = true;
}


ActionProgressPtr ProgressDisplayer::forkNewAction(
    double nSteps, const std::string& name)
{
    ActionProgressPtr ap(new ActionProgress(this, name, nSteps));
    ap->startAction();
    return ap;
}


ActionProgress* ProgressDisplayer::findAction(const std::string& pathstr)
{
    ElementPath ep(pathstr);

    insight::assertion(!ep.empty(), "empty action path");

    const auto rootName = ep.front();
    ep.erase(ep.begin());

    std::lock_guard<std::mutex> lock(childActionsMutex_);

    auto i = std::find_if(
        childActions_.begin(), childActions_.end(),
        [&rootName](ActionProgress* ap)
        { return ap->name() == rootName; });

    if (i == childActions_.end())
        return nullptr;

    if (ep.empty())
        return *i;

    return (*i)->getChildAction(ep);
}


void ProgressDisplayer::triggerStop(const std::string &actionPath)
{
    if (auto *a=findAction(actionPath))
        a->triggerStop();
}




bool ProgressDisplayer::stopIsDemanded() const
{
  return stopTriggered_;
}




} // namespace insight
