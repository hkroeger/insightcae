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
};




class AngleConstraint
: public Angle,
  public ConstrainedSketchEntity
{

    size_t calcHash() const override;

protected:
    AngleConstraint(
        VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
        const std::string& layerName = std::string());

public:
    declareType("AngleConstraintBase");

    virtual double targetValue() const =0;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;


    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    double dimLineRadius() const override;
    void setDimLineRadius(double r);

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const AngleConstraint& other);
};




class FixedAngleConstraint
: public AngleConstraint
{

    FixedAngleConstraint(
        VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
        const std::string& layerName = std::string());

public:
    declareType("AngleConstraint");

    CREATE_FUNCTION(FixedAngleConstraint);


    double targetValue() const override;
    void setTargetValue(double angle);

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        MakeDefaultGeometryParametersFunction mdpf );

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const FixedAngleConstraint& other);
};




class LinkedAngleConstraint
    : public AngleConstraint
{
    std::string angleExpr_;
    ScalarPtr angle_;

    LinkedAngleConstraint(
        VectorPtr p1, VectorPtr p2, VectorPtr pCtr,
        ScalarPtr angle,
        const std::string& layerName = std::string(),
        const std::string& angleExpr = std::string() );

public:
    declareType("Angle");

    CREATE_FUNCTION(LinkedAngleConstraint);

    double targetValue() const override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        MakeDefaultGeometryParametersFunction mdpf );

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const LinkedAngleConstraint& other);
};



} // namespace cad
} // namespace insight

#endif // INSIGHT_CAD_ANGLE_H
