#ifndef INSIGHT_BMD_WEDGE2D_H
#define INSIGHT_BMD_WEDGE2D_H


#include "openfoam/blockmesh/transform2d.h"

namespace insight {
namespace bmd {

class wedge2D
: public transform2D
{
  arma::mat fwdrot_, rvsrot_;

public:
    wedge2D(int idx=2);
    virtual ~wedge2D();

    virtual arma::mat fwd(const arma::mat& p) const;
    virtual arma::mat rvs(const arma::mat& p) const;
};


} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_WEDGE2D_H
