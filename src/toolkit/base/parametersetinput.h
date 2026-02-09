#ifndef PARAMETERSETINPUT_H
#define PARAMETERSETINPUT_H


#include "base/parametersbase.h"


namespace insight {


/**
 * @brief The ParameterSetInput class
 * A ParameterSetInput object stores a Parameters object
 * optionally along with a reference to its source.
 */
struct ParameterSetInput
{
private:
    /**
     * @brief ps_
     * optional source of the Parameters object
     */
    std::observer_ptr<const ParameterSet> ps_;

    /**
     * @brief p_
     * storage of the Parameters object
     */
    std::unique_ptr<insight::ParametersBase> p_;

public:

    ParameterSetInput();
    ParameterSetInput(ParameterSetInput&& o);
    ParameterSetInput(const ParameterSet& subs);
    ParameterSetInput(const ParameterSet* ps, std::unique_ptr<insight::ParametersBase>&& p );

    // store a copy of given parameters without source parameter set
    ParameterSetInput( const insight::ParametersBase& p );

    template<class P>
    // std::unique_ptr<insight::ParametersBase>
    std::unique_ptr<insight::ParametersBase>
    create()
    {
        if (p_)
            return std::move(p_);
        else
            return std::make_unique<P>(parameterSet());
    }

    template<class P>
    // std::unique_ptr<insight::ParametersBase>
    ParameterSetInput
    forward()
    {
        // forward_visitor<P> v;
        // return boost::apply_visitor(v, *this);
        return ParameterSetInput(ps_.valid()?ps_.get():nullptr, create<P>() );
    }

    // void operator=(ParameterSetInput& other);
    ParameterSetInput& operator=(ParameterSetInput&& o);

    bool hasParameters() const;
    const ParametersBase& parameters() const;

    template<class P>
    const P& pAs() const
    {
        return dynamic_cast<const P&>(*p_);
    }

    /**
     * @brief tweakParameters
     * returns a writable reference to the parameters.
     * Invalidates the parameter set pointer, since it is out of sync after modification
     * @return
     * a reference to the parameters
     */
    ParametersBase& tweakParameters();

    std::unique_ptr<insight::ParametersBase> moveParameters();

    bool hasParameterSet() const;
    const ParameterSet& parameterSet() const;
    std::unique_ptr<ParameterSet> parameterSetCopy() const;

    ParameterSetInput clone() const;

private:
    ParameterSetInput( const ParameterSetInput& o ) = delete;
    ParameterSetInput& operator=( const ParameterSetInput& o ) = delete;
};



} // namespace insight

#endif // PARAMETERSETINPUT_H
