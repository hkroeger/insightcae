#ifndef PARAMETERSBASE_H
#define PARAMETERSBASE_H


#include "base/parameterset.h"

#include <memory>




namespace insight {



struct ParametersBase
{
    ParametersBase();
    ParametersBase(const insight::ParameterSet& p);
    virtual ~ParametersBase();

    virtual void set(insight::ParameterSet& p) const;
    virtual void get(const insight::ParameterSet& p);

    static std::unique_ptr<ParameterSet> makeDefault();

    virtual std::unique_ptr<ParameterSet> cloneParameterSet() const =0;

    virtual std::unique_ptr<ParametersBase> clone() const =0;
};



} // namespace insight

#endif // PARAMETERSBASE_H
