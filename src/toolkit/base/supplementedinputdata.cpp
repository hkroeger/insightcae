#include "supplementedinputdata.h"
#include "base/exception.h"
#include "base/parameterset.h"
#include <string>

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






ParameterSetInput::ParameterSetInput()
    // : ParameterSetInputBase(
    //       static_cast<const ParameterSet*>(
    //           nullptr ) )
{}

ParameterSetInput::ParameterSetInput(ParameterSetInput &&o)
    : ps_(o.ps_)
    // : ParameterSetInputBase(std::move(o))
{
    if (o.hasParameters())
    {
        p_=std::move(o.p_);
    }
}

ParameterSetInput::ParameterSetInput(const ParameterSet &ps)
    : ps_(&ps)
    // : ParameterSetInputBase(&ps)
{}


ParameterSetInput::ParameterSetInput(
    const ParameterSet* ps,
    std::unique_ptr<insight::ParametersBase>&& p )

    : ps_(ps), p_(std::move(p))
{}

ParameterSetInput::ParameterSetInput(
    const insight::ParametersBase& p )
    : p_(p.clone())
{}


ParameterSetInput& ParameterSetInput::operator=(ParameterSetInput&& o)
{
    // struct assign_visitor
    //     : public boost::static_visitor<void>
    // {
    //     ParameterSetInput& psi_;
    //     assign_visitor(ParameterSetInput&psi) : psi_(psi) {}

    //     void operator()( std::unique_ptr<insight::ParametersBase>& pb )
    //     {
    //         ParameterSetInput ip(std::move(pb));
    //         psi_.swap(ip);
    //     }

    //     void
    //     operator()( const ParameterSet* ps )
    //     {
    //         ParameterSetInput ip(ps);
    //         psi_.swap(ip);
    //     }

    //     void
    //     operator()(std::unique_ptr<insight::ParameterSet>& ps)
    //     {
    //         ParameterSetInput ip(std::move(ps));
    //         psi_.swap(ip);
    //     }
    // } av(*this);

    // boost::apply_visitor(av, *this);
    // return *this;

    ps_=(o.ps_);

    if (o.hasParameters())
    {
        p_=o.moveParameters();
    }

    return *this;
}


bool ParameterSetInput::hasParameters() const
{
    return bool(p_);
}


const ParametersBase &ParameterSetInput::parameters() const
{
    insight::assertion(
        hasParameters(),
        "there have been no parameters stored" );

    return *p_;
}

ParametersBase &ParameterSetInput::tweakParameters()
{
    ps_ = std::observer_ptr<const ParameterSet>(nullptr);
    return *p_;
}


std::unique_ptr<ParametersBase> ParameterSetInput::moveParameters()
{
    insight::assertion(
        hasParameters(),
        "there have been no parameters stored" );

    return std::move(p_);
}





bool ParameterSetInput::hasParameterSet() const
{
    return bool(ps_);
}



const ParameterSet &ParameterSetInput::parameterSet() const
{
    insight::assertion(
        hasParameterSet(),
        "there has been no parameter set stored" );

    return *ps_;
}



std::unique_ptr<ParameterSet> ParameterSetInput::parameterSetCopy() const
{
    auto &ps = parameterSet();

    if (auto* rps=dynamic_cast<const ParameterSet*>(&ps))
        return rps->cloneParameterSet();
    else
        return ParameterSet::create(ps.entries(), std::string());
}

ParameterSetInput ParameterSetInput::clone() const
{
    return ParameterSetInput(ps_.get(), p_->clone());
}







supplementedInputDataBase::supplementedInputDataBase(
    const boost::filesystem::path &exePath )
    : executionPath_(exePath)
{}

supplementedInputDataBase::supplementedInputDataBase(
    ParameterSetInput ip,
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
