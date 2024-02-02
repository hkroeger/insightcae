#ifndef INSIGHT_BMD_TRANSFORM2D_H
#define INSIGHT_BMD_TRANSFORM2D_H

#include "openfoam/blockmesh/point.h"
#include "openfoam/blockmesh/patch.h"


namespace insight {
namespace bmd {


class blockMesh;

class transform2D
{
protected:
  int idx_;
  std::vector<int> map_;
  std::vector<int> dir_;

  Patch fwdPatch_, rvsPatch_;

public:
    transform2D(int idx=2);
    virtual ~transform2D();

    arma::mat mapped3D(const arma::mat& p) const;

    virtual arma::mat fwd(const arma::mat& p) const =0;
    virtual arma::mat rvs(const arma::mat& p) const =0;

    inline Patch& fwdPatch() { return fwdPatch_; }
    inline Patch& rvsPatch() { return rvsPatch_; }

    void addFwdRvsPatches(blockMesh *bmd);
};



} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_TRANSFORM2D_H
