#include "parametersetinput.h"

namespace insight {




ParameterSetInput::ParameterSetInput()
{}




ParameterSetInput::ParameterSetInput(ParameterSetInput &&o)
    : ps_(o.ps_)
{
    if (o.hasParameters())
    {
        p_=std::move(o.p_);
    }
}




ParameterSetInput::ParameterSetInput(const ParameterSet &ps)
    : ps_(&ps)
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
        return rps->cloneAs<ParameterSet>();
    else
        return ParameterSet::create(ps.entries(), std::string());
}




ParameterSetInput ParameterSetInput::clone() const
{
    return ParameterSetInput(
        ps_.valid() ? ps_.get() : nullptr,
        p_->clone() );
}




} // namespace insight
