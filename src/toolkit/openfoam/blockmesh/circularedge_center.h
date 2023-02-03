#ifndef INSIGHT_BMD_CIRCULAREDGE_CENTER_H
#define INSIGHT_BMD_CIRCULAREDGE_CENTER_H

#include "openfoam/blockmesh/arcedge.h"

namespace insight {
namespace bmd {

class CircularEdge_Center
: public ArcEdge
{
public:
    CircularEdge_Center
    (
      const Point& c0, const Point& c1,
      const Point& center
    );
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_CIRCULAREDGE_CENTER_H
