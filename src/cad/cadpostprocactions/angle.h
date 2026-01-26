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

  Angle(VectorPtr p1, VectorPtr p2, VectorPtr pCtr);

public:
  declareType("ShowAngle");

  CREATE_FUNCTION(Angle);

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

  arma::mat symbolLocation() const;
  std::vector<vtkSmartPointer<vtkProp> > createVTKRepr() const override;

  static void insertrule(parser::ISCADParser& ruleset);
};


} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ANGLE_H
