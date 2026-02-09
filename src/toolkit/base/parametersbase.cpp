#include "parametersbase.h"




namespace insight {




ParametersBase::ParametersBase()
{}




ParametersBase::ParametersBase(const insight::ParameterSet& /*p*/)
{}




ParametersBase::~ParametersBase()
{}




void ParametersBase::set(ParameterSet &p) const
{}




void ParametersBase::get(const ParameterSet &p)
{}




std::unique_ptr<ParameterSet> ParametersBase::makeDefault()
{
    return ParameterSet::create();
}




} // namespace insight
