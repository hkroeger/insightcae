#ifndef INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
#define INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H

#include <memory>

namespace insight {
namespace cad {

class ConstrainedSketchGeometry
{
public:
    virtual int nDoF() const;
    virtual double getDoFValue(unsigned int iDoF) const;
    virtual void setDoFValue(unsigned int iDoF, double value);

    virtual int nConstraints() const;
    virtual double getConstraintError(unsigned int iConstraint) const;
};

typedef
    std::shared_ptr<ConstrainedSketchGeometry>
    ConstrainedSketchGeometryPtr;


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_CONSTRAINEDSKETCHGEOMETRY_H
