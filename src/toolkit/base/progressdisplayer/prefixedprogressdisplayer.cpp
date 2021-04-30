#include "prefixedprogressdisplayer.h"

namespace insight
{


std::string PrefixedProgressDisplayer::prefixedPVPath(const std::string& path) const
{
  std::string p = path;
  if (actionPrefix_==HierarchyLevelPrefix)
   p=prefix_+"/"+p;
  else if (actionPrefix_==ParallelPrefix)
   p=prefix_+":"+p;
  return p;
}



void PrefixedProgressDisplayer::setActionProgressValue(const std::string &path, double value)
{
  parent_->setActionProgressValue( prefixedPVPath(path), value);
}

void PrefixedProgressDisplayer::setMessageText(const std::string &path, const std::string &message)
{

  parent_->setMessageText( prefixedPVPath(path), message);
}

void PrefixedProgressDisplayer::finishActionProgress(const std::string &path)
{
  parent_->finishActionProgress( prefixedPVPath(path) );
}





PrefixedProgressDisplayer::PrefixedProgressDisplayer
(
    ProgressDisplayer* parent, const std::string& prefix,
    ProgressVariablePrefixType progressVariablePrefix,
    ActionProgressPrefixType actionPrefix
    )
  : parent_(parent), prefix_(prefix),
    pvPrefix_(progressVariablePrefix),
    actionPrefix_(actionPrefix)
{
}




void PrefixedProgressDisplayer::update (
    const ProgressState& pi
    )
{
  std::string p;
  if (pvPrefix_==Prefixed)
    p=prefix_+"/";

  ProgressVariableList pvl;
  for (const auto& pv: pi.second)
  {
    pvl[p+pv.first]=pv.second;
  }
  parent_->update( ProgressState(pi.first, pvl, pi.logMessage_) );
}


void PrefixedProgressDisplayer::reset()
{}


bool PrefixedProgressDisplayer::stopRun() const
{
  return parent_->stopRun();
}




}
