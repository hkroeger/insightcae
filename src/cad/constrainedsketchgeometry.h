#ifndef INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
#define INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H

#include <memory>
#include <string>

#include "base/parameterset.h"

namespace insight {
namespace cad {

class ConstrainedSketchEntity
{
    insight::ParameterSet parameters_, defaultParameters_;

public:
    virtual int nDoF() const;
    virtual double getDoFValue(unsigned int iDoF) const;
    virtual void setDoFValue(unsigned int iDoF, double value);

    virtual int nConstraints() const;
    virtual double getConstraintError(unsigned int iConstraint) const;

    virtual size_t hash() const;

    const insight::ParameterSet& parameters() const;
    insight::ParameterSet& parametersRef();
    const insight::ParameterSet& defaultParameters() const;
    void changeDefaultParameters(const insight::ParameterSet&);
};

typedef
    std::shared_ptr<ConstrainedSketchEntity>
    ConstrainedSketchEntityPtr;


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
