#ifndef INSIGHT_CAD_ANGLE_H
#define INSIGHT_CAD_ANGLE_H

#include "cadtypes.h"
#include "cadpostprocaction.h"
#include "constrainedsketchgeometry.h"


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
  declareType("Angle");

  Angle(VectorPtr p1, VectorPtr p2, VectorPtr pCtr);

  static double calculate(
          arma::mat p1,
          arma::mat p2,
          arma::mat pCtr
          );

  void build() override;

  void write(std::ostream&) const override;
};




class AngleConstraint
: public Angle,
  public ConstrainedSketchGeometry
{
    ScalarPtr targetValue_;

    size_t calcHash() const override;

public:
    declareType("AngleConstraint");

    AngleConstraint(VectorPtr p1, VectorPtr p2, VectorPtr pCtr, ScalarPtr targetValue);

    ScalarPtr targetValue();

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
};




} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ANGLE_H
