#include "transform2d.h"
#include "openfoam/blockmesh.h"

#include "boost/assign/std/vector.hpp"

using namespace boost::assign; // for operator+=

namespace insight {
namespace bmd {



transform2D::transform2D(int idx)
: idx_(idx),
  map_(),
  dir_()
{
    if (idx==0)
    {
    map_+=1,2; //<<1<<2; //[1, 2]
    dir_+=1,1,1; //<<1<<1<<1; //[1., 1., 1.]
    }
    else if (idx==1)
    {
    map_+=2,0; //<<2<<0; //[2, 0]
    dir_+=1,1,-1; //<<1<<1<<-1; //[1., 1., -1.]
    }
    else if (idx==2)
    {
    map_+=0,1; //<<0<<1; //[0, 1]
    dir_+=1,1,1; //<<1<<1<<1; //[1., 1., 1.]
    }
}

transform2D::~transform2D()
{}

arma::mat transform2D::mapped3D(const arma::mat& p) const
{
  arma::mat p3=vec3(0., 0., 0.);
  p3[map_[0]]=dir_[0]*p[0];
  p3[map_[1]]=dir_[1]*p[1];
  return p3;
}



void transform2D::addFwdRvsPatches(blockMesh* bmd)
{
    bmd->addPatch("front", new Patch(fwdPatch_));
    bmd->addPatch("back", new Patch(rvsPatch_));
}



} // namespace bmd
} // namespace insight
