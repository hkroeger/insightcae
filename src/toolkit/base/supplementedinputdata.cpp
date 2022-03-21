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

void supplementedInputDataBase::reportSupplementQuantity(
    const std::string &name,
    ReportedSupplementQuantityValue value,
    const std::string &description,
    const std::string &unit )
{
  reportedSupplementQuantities_.insert( std::make_pair(
        name,
        ReportedSupplementQuantity{ value, description, unit }
                                          ) );
}

const supplementedInputDataBase::ReportedSupplementQuantitiesTable &supplementedInputDataBase::reportedSupplementQuantities() const
{
  return reportedSupplementQuantities_;
}

} // namespace insight
