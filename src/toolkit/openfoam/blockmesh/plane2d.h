#ifndef INSIGHT_BMD_PLANE2D_H
#define INSIGHT_BMD_PLANE2D_H

#include "openfoam/blockmesh/transform2d.h"

namespace insight {
namespace bmd {

class plane2D
: public transform2D
{
protected:
  arma::mat ofs_;

public:
    plane2D(double thick, int idx=2);
    virtual ~plane2D();

    virtual arma::mat fwd(const arma::mat& p) const;
    virtual arma::mat rvs(const arma::mat& p) const;
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PLANE2D_H
