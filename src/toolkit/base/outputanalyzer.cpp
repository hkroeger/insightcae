#include "outputanalyzer.h"

#include "base/progressdisplayer.h"

namespace insight {


OutputAnalyzer::OutputAnalyzer(ProgressDisplayer *progress)
  : progress_(progress)
{}


OutputAnalyzer::~OutputAnalyzer()
{}



bool OutputAnalyzer::stopIsDemanded() const
{
  if (progress_)
    return progress_->stopIsDemanded();
  else
    return false;
}



} // namespace insight
