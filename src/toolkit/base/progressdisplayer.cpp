#include "progressdisplayer.h"



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







std::string ProgressDisplayer::actionPath() const
{
  return "";
}

ProgressDisplayer::~ProgressDisplayer()
{
}

ActionProgress ProgressDisplayer::forkNewAction(double nSteps, const std::string& name)
{
  return ActionProgress(*this, name, nSteps);
}

void ProgressDisplayer::stepUp(double steps)
{
  ci_+=steps;
  setActionProgressValue(actionPath(), ci_/(maxi_+1));
}

void ProgressDisplayer::stepTo(double i)
{
  ci_=i;
  setActionProgressValue(actionPath(), ci_/(maxi_+1));
}

void ProgressDisplayer::operator++()
{
  stepUp();
}

void ProgressDisplayer::operator+=(double n)
{
  stepUp(n);
}

void ProgressDisplayer::message(const std::string &message)
{
  setMessageText(actionPath(), message);
}




bool ProgressDisplayer::stopRun() const
{
  return false;
}


void ActionProgress::setActionProgressValue(const std::string &path, double value)
{
  parentAction_.setActionProgressValue(path, value);
}

void ActionProgress::setMessageText(const std::string &path, const std::string &message)
{
  parentAction_.setMessageText(path, message);
}

void ActionProgress::finishActionProgress(const std::string &path)
{
  parentAction_.finishActionProgress(path);
}

void ActionProgress::update(const ProgressState &pi)
{
  parentAction_.update(pi);
}

void ActionProgress::reset()
{
  // ignore
}

std::string ActionProgress::actionPath() const
{
  std::string p = name_;

  if (!parentAction_.actionPath().empty())
    p=parentAction_.actionPath()+"/"+p;

  return p;
}

ActionProgress::ActionProgress
(
    const ProgressDisplayer &parentAction,
    std::string name,
    double nSteps
)
  : parentAction_(const_cast<ProgressDisplayer&>(parentAction)),
    name_(name)
{
  maxi_=nSteps;
}


ActionProgress::~ActionProgress()
{
  finishActionProgress(name_);
}



} // namespace insight
