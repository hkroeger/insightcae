#include "constrainedsketchgeometry.h"

#include "base/exception.h"


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




} // namespace cad
} // namespace insight
