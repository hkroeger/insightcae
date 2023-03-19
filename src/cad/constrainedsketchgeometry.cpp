#include "constrainedsketchgeometry.h"

#include "base/exception.h"

#include "boost/functional/hash.hpp"

namespace insight {
namespace cad {



int ConstrainedSketchEntity::nDoF() const
{
    return 0;
}

double ConstrainedSketchEntity::getDoFValue(unsigned int iDoF) const
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
    return NAN;
}

void ConstrainedSketchEntity::setDoFValue(unsigned int iDoF, double value)
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
}

int ConstrainedSketchEntity::nConstraints() const
{
    return 0;
}

double ConstrainedSketchEntity::getConstraintError(unsigned int iConstraint) const
{
    throw insight::Exception("invalid constraint index: %d", iConstraint);
    return NAN;
}

size_t ConstrainedSketchEntity::hash() const
{
    size_t h=0;
    for (int i=0; i<nDoF(); ++i)
        boost::hash_combine( h, boost::hash<double>()(getDoFValue(i)) );
    return h;
}


insight::ParameterSet& ConstrainedSketchEntity::parameters()
{
    return parameters_;
}

const insight::ParameterSet& ConstrainedSketchEntity::defaultParameters() const
{
    return defaultParameters_;
}

void ConstrainedSketchEntity::changeDefaultParameters(const insight::ParameterSet& ps)
{
    defaultParameters_=ps;
    ParameterSet oldps=parameters_;
    parameters_=defaultParameters_;

#warning copy values from old; merge is not the right function
    //parameters_.merge(oldps);
}



} // namespace cad
} // namespace insight
