#include "parametersetvisualizer.h"

#include "base/exception.h"
#include "base/parameterset.h"

namespace insight {


bool ParameterSetVisualizer::hasScheduledParameters() const
{
  return bool(scheduledParameters_);
}

bool ParameterSetVisualizer::hasCurrentParameters() const
{
  return bool(visualizedParameters_);
}

const ParameterSet &ParameterSetVisualizer::currentParameters() const
{
  if (visualizedParameters_)
    return *visualizedParameters_;
  else
    throw insight::Exception("internal error: no parameters selected for visualization!");
}

bool ParameterSetVisualizer::selectScheduledParameters()
{
  if (!visualizedParameters_)
  {
    if (scheduledParameters_)
    {
          CurrentExceptionContext ex("selecting new parameter set for next visualization");

          visualizedParameters_.reset(
              scheduledParameters_->cloneParameterSet() );
          scheduledParameters_=nullptr;

          return true;
      }
  }
  return false;
}

void ParameterSetVisualizer::clearScheduledParameters()
{
  CurrentExceptionContext ex("clearing parameter set for visualization");
  visualizedParameters_.reset();
}

ParameterSetVisualizer::ParameterSetVisualizer()
  : defaultProgressDisplayer_(),
    progress_(&defaultProgressDisplayer_),
    scheduledParameters_(nullptr)
{}

ParameterSetVisualizer::~ParameterSetVisualizer()
{}

void ParameterSetVisualizer::update(const ParameterSet& ps)
{
  CurrentExceptionContext ex("scheduling parameters for visualization");
  scheduledParameters_=&ps;
}




void ParameterSetVisualizer::setIcon(QIcon*)
{
}

void ParameterSetVisualizer::setProgressDisplayer(ProgressDisplayer* pd)
{
  if (pd!=nullptr)
    progress_=pd;
  else
    progress_=&defaultProgressDisplayer_;
}

CameraState ParameterSetVisualizer::defaultCameraState() const
{
    return CameraState{
        true, 1.,
        vec3(1,1,1), vec3Zero(), vec3Z(1)
    };
}



} // namespace insight
