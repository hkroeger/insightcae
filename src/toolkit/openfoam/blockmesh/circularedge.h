#ifndef INSIGHT_BMD_CIRCULAREDGE_H
#define INSIGHT_BMD_CIRCULAREDGE_H

#include "openfoam/blockmesh/arcedge.h"

namespace insight {
namespace bmd {


class CircularEdge
: public ArcEdge
{
public:
    CircularEdge
    (
      const Point& c0, const Point& c1,
      Point startPoint=vec3(0,0,0),
      arma::mat axis=vec3(0,0,1)
    );
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_CIRCULAREDGE_H
