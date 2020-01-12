#include "prefixedprogressdisplayer.h"

namespace insight
{


PrefixedProgressDisplayer::PrefixedProgressDisplayer(ProgressDisplayer* parent, const std::string& prefix)
  : parent_(parent), prefix_(prefix)
{
}

void PrefixedProgressDisplayer::update (
    const ProgressState& pi
    )
{
  ProgressVariableList pvl;
  for (const auto& pv: pi.second)
  {
    pvl[prefix_+pv.first]=pv.second;
  }
  parent_->update( ProgressState(pi.first, pvl, pi.logMessage_) );
}

bool PrefixedProgressDisplayer::stopRun() const
{
  return parent_->stopRun();
}


}
