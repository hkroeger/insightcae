#ifndef INSIGHT_CADGEOMETRYPARAMETER_H
#define INSIGHT_CADGEOMETRYPARAMETER_H

#include "cadtypes.h"
#include "base/parameters/pathparameter.h"

namespace insight {

class CADGeometryParameter
        : public Parameter
{
public:
    template<class ...Args>
    CADGeometryParameter(Args&&... addArgs)
        : Parameter( std::forward<Args>(addArgs)... )
    {}

    virtual cad::FeaturePtr geometry() const =0;
};


} // namespace insight

#endif // INSIGHT_CADGEOMETRYPARAMETER_H
