#ifndef INSIGHT_BMD_BLOCK_H
#define INSIGHT_BMD_BLOCK_H


#include "openfoam/blockmesh/point.h"

namespace insight {
namespace bmd {




class blockMesh;



class Block
{
public:
  typedef std::vector<double> EdgeGradList;
  typedef boost::variant<
      double,
      EdgeGradList
      > Grading;
  typedef std::vector<Grading> GradingList;

protected:
  PointList corners_;
  std::vector<int> resolution_;
  GradingList grading_;
  std::string zone_;
  bool inv_;

public:
  Block(PointList corners,
    int resx, int resy, int resz,
    GradingList grading = GradingList(3, 1),
    std::string zone="",
    bool inv=false);
  virtual ~Block();

  void registerPoints(blockMesh& bmd) const;

  PointList face(const std::string& id) const;

  void swapGrad();

  std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const;

  Block* transformed(const arma::mat& tm, bool inv=false, const arma::mat trans=vec3(0,0,0)) const;
  virtual Block* clone() const;

  inline int nCells() const
  {
    return resolution_[0]*resolution_[1]*resolution_[2];
  }

  const PointList& corners() const;

};



} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCK_H
