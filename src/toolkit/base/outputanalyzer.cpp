#include "outputanalyzer.h"

#include "base/progressdisplayer.h"

namespace insight {


OutputAnalyzer::OutputAnalyzer(ProgressDisplayer *progress)
  : progress_(progress)
{}


OutputAnalyzer::~OutputAnalyzer()
{}



bool OutputAnalyzer::stopRun() const
{
  if (progress_)
    return progress_->stopRun();
  else
    return false;
}



} // namespace insight
