#ifndef INSIGHT_BMD_PATCH_H
#define INSIGHT_BMD_PATCH_H

#include "openfoam/blockmesh/point.h"

namespace insight {
namespace bmd {

class Patch
{
public:
  typedef std::vector<PointList> FaceList;

protected:
  std::string typ_;
  FaceList faces_;

public:
  Patch(std::string typ="patch");
  virtual ~Patch();

  void addFace(Point c1, Point c2, Point c3, Point c4);
  void addFace(const PointList& corners);

  void appendPatch(const Patch& opatch);

  /**
   * remove all faces from current patch
   */
  void clear();

  Patch* transformed(const arma::mat& tm, bool inv=false, const arma::mat trans=vec3(0,0,0) ) const;
  virtual Patch* clone() const;

  inline const FaceList& faces() const { return faces_; }

  std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, const std::string& name, int OFversion) const;

};

} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_PATCH_H
