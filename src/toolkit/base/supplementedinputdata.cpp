#include "supplementedinputdata.h"
#include "base/cppextensions.h"
#include "base/exception.h"
#include "base/parameterset.h"
#include <string>

namespace insight {







supplementedInputDataBase::supplementedInputDataBase(
    const boost::filesystem::path &exePath )
    : executionPath_(exePath)
{}

supplementedInputDataBase::supplementedInputDataBase(
    ParameterSetInput&& ip,
    const boost::filesystem::path& exePath,
    ProgressDisplayer& )
  : executionPath_(exePath)
{
#warning check, if can be avoided
    parameters_ = &ip.parameterSet();
}

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


void supplementedInputDataBase::insertSupplementQuantities(
    const std::string &prefix,
    const ReportedSupplementQuantitiesTable &rsqt)
{
    std::transform(
        rsqt.begin(), rsqt.end(),
        std::inserter(reportedSupplementQuantities_, reportedSupplementQuantities_.end()),
        [&](const ReportedSupplementQuantitiesTable::value_type& o)
        {
            auto newname=o.first;
            if (!prefix.empty()) newname=prefix+"/"+newname;
            return ReportedSupplementQuantitiesTable::value_type{
                newname, o.second };
        }
        );
}

const ParameterSet& supplementedInputDataBase::parameters() const
{
    return *parameters_;
}




supplementedInputDataFromParameters::supplementedInputDataFromParameters(
    ParameterSetInput ip,
    const boost::filesystem::path& exePath,
    ProgressDisplayer& progress)
: std::unique_ptr<ParametersBase>(ip.moveParameters()),
    supplementedInputDataBase(std::move(ip), exePath, progress)
{}




const boost::filesystem::path &supplementedInputDataBase::executionPath() const
{
    return executionPath_;
}



const supplementedInputDataBase::ReportedSupplementQuantitiesTable&
supplementedInputDataBase::reportedSupplementQuantities() const
{
  return reportedSupplementQuantities_;
}



} // namespace insight
