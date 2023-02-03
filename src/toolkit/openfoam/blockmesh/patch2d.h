#ifndef INSIGHT_BMD_PATCH2D_H
#define INSIGHT_BMD_PATCH2D_H

#include "openfoam/blockmesh/patch.h"
#include "openfoam/blockmesh/transform2d.h"

namespace insight {
namespace bmd {

class Patch2D
: public Patch
{
protected:
  const transform2D& t2d_;

public:
    Patch2D(const transform2D& t2d, std::string typ="patch");
    void addFace(const Point& c0, const Point& c1);
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PATCH2D_H
