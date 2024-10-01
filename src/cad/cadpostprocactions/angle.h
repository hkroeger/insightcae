#ifndef INSIGHT_CAD_ANGLE_H
#define INSIGHT_CAD_ANGLE_H

#include "cadtypes.h"
#include "cadpostprocaction.h"


namespace insight {
namespace cad {


class Angle
: public PostprocAction
{

  size_t calcHash() const override;

public:
  VectorPtr p1_, p2_, pCtr_;
  double angle_;

public:
  declareType("ShowAngle");

  Angle(VectorPtr p1, VectorPtr p2, VectorPtr pCtr);

  static double calculate(
          arma::mat p1,
          arma::mat p2,
          arma::mat pCtr
          );

  void build() override;

  void write(std::ostream&) const override;

  virtual double dimLineRadius() const;
  virtual double relativeArrowSize() const;

  VectorPtr centerPoint() const;

  void operator=(const Angle& other);

  std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const override;
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ANGLE_H
