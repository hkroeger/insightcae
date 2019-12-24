#include "progressdisplayer.h"



namespace insight {



ProgressDisplayer::~ProgressDisplayer()
{
}

bool ProgressDisplayer::stopRun() const
{
  return false;
}

ProgressState::ProgressState(
    double t,
    ProgressVariableList pvl,
    const std::string &message
    )
  : std::pair<double, ProgressVariableList>(t, pvl),
    logMessage_(message)
{}



} // namespace insight
