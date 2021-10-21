#include "supplementedinputdata.h"

namespace insight {


ParametersBase::ParametersBase()
{}

ParametersBase::ParametersBase(const insight::ParameterSet& /*p*/)
{}

ParametersBase::~ParametersBase()
{}

ParameterSet ParametersBase::makeDefault()
{
  return ParameterSet();
}




supplementedInputDataBase::supplementedInputDataBase(std::unique_ptr<ParametersBase> pPtr)
  : std::unique_ptr<ParametersBase>( std::move(pPtr) )
{}

supplementedInputDataBase::~supplementedInputDataBase()
{}

} // namespace insight
