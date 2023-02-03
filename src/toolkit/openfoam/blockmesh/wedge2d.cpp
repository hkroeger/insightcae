#include "wedge2d.h"

namespace insight {
namespace bmd {




wedge2D::wedge2D(int idx)
: transform2D(idx)
{
    arma::mat ax=vec3(0., 0., 0.);
    ax[map_[0]]=1.0;
    fwdrot_=rotMatrix(2.5*M_PI/180., ax);
    rvsrot_=rotMatrix(-2.5*M_PI/180., ax);
    fwdPatch_=Patch("wedge");
    rvsPatch_=Patch("wedge");
}

wedge2D::~wedge2D()
{}

arma::mat wedge2D::fwd(const arma::mat& p) const
{
    return fwdrot_*mapped3D(p);
}

arma::mat wedge2D::rvs(const arma::mat& p) const
{
    return rvsrot_*mapped3D(p);
}



} // namespace bmd
} // namespace insight
