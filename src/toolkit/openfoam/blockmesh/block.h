#ifndef INSIGHT_BMD_BLOCK_H
#define INSIGHT_BMD_BLOCK_H


#include "openfoam/blockmesh/point.h"

#include <array>

namespace insight {
namespace bmd {




class blockMeshBlocking;



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

  void registerPoints(blockMeshBlocking& bmd) const;

  PointList face(const std::string& id) const;

  void swapGrad();

  std::vector<OFDictData::data>
  bmdEntry(const PointMap& allPoints, int OFversion) const;

  Block* transformed(const arma::mat& tm, bool inv=false, const arma::mat trans=vec3(0,0,0)) const;
  virtual Block* clone() const;

  /**
   * @brief orderedCorners
   * the corners in the order, in which they are written into the blockMeshDict
   */
  PointList orderedCorners() const;

  inline const std::vector<int>& resolution() const { return resolution_; }

  /**
   * @brief edgeGradings
   * the expansion ratios of all 12 edges in OpenFOAM edge order:
   * x1: 0-1, 3-2, 7-6, 4-5; x2: 0-3, 1-2, 5-6, 4-7; x3: 0-4, 1-5, 2-6, 3-7
   */
  std::array<double,12> edgeGradings() const;

  inline int nCells() const
  {
    return resolution_[0]*resolution_[1]*resolution_[2];
  }

  const PointList& corners() const;

  /**
   * replace the corner coordinates (in the order of corners())
   */
  void setCorners(const PointList& corners);

};



} // namespace bmd
} // namespace insight

#endif // INSIGHT_BMD_BLOCK_H
