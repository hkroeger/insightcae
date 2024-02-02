#include "plane2d.h"

namespace insight {
namespace bmd {


plane2D::plane2D(double thick, int idx)
: transform2D(idx)
{
  ofs_=vec3(0, 0, 0);
  ofs_[idx_]=0.5*thick*dir_[2];
  fwdPatch_=Patch("empty");
  rvsPatch_=Patch("empty");
}

plane2D::~plane2D()
{}

arma::mat plane2D::fwd(const arma::mat& p) const
{
    return mapped3D(p)+ofs_;
}

arma::mat plane2D::rvs(const arma::mat& p) const
{
    return mapped3D(p)-ofs_;
}


} // namespace bmd
} // namespace insight
