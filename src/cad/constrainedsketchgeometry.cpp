#include "constrainedsketchgeometry.h"

#include "base/exception.h"

#include "boost/functional/hash.hpp"

namespace insight {
namespace cad {



int ConstrainedSketchGeometry::nDoF() const
{
    return 0;
}

double ConstrainedSketchGeometry::getDoFValue(unsigned int iDoF) const
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
    return NAN;
}

void ConstrainedSketchGeometry::setDoFValue(unsigned int iDoF, double value)
{
    throw insight::Exception("invalid DoF index: %d", iDoF);
}

int ConstrainedSketchGeometry::nConstraints() const
{
    return 0;
}

double ConstrainedSketchGeometry::getConstraintError(unsigned int iConstraint) const
{
    throw insight::Exception("invalid constraint index: %d", iConstraint);
    return NAN;
}

size_t ConstrainedSketchGeometry::hash() const
{
    size_t h=0;
    for (int i=0; i<nDoF(); ++i)
        boost::hash_combine( h, boost::hash<double>()(getDoFValue(i)) );
    return h;
}




} // namespace cad
} // namespace insight
