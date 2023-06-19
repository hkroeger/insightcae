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
  public ConstrainedSketchEntity
{

    size_t calcHash() const override;

    AngleConstraint(VectorPtr p1, VectorPtr p2, VectorPtr pCtr, double targetValue);

public:
    declareType("AngleConstraint");

    CREATE_FUNCTION(AngleConstraint);

    double targetValue() const;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(ConstrainedSketchGrammar& ruleset, MakeDefaultGeometryParametersFunction mdpf);

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;
};




} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ANGLE_H
