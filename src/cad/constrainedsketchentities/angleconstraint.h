#ifndef ANGLECONSTRAINT_H
#define ANGLECONSTRAINT_H

#include "constraintwithdimensionlines.h"
#include "cadpostprocactions/angle.h"

namespace insight {
namespace cad {


class AngleConstraint
    : public Angle,
      public ConstraintWithDimensionLines
{

    size_t calcHash() const override;

protected:
    /**
     * @brief AngleConstraint
     * @param p1
     * @param p2
     * if nullptr, measurement is against horizontal direction
     * @param pCtr
     * @param layerName
     */
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

    double relativeArrowSize() const override;
    void setArrowSize(double absoluteArrowSize) override;

    bool isInside( SelectionRect r) const override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const AngleConstraint& other);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;
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
        const ConstrainedSketchParametersDelegate& pd );

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const FixedAngleConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
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
        const ConstrainedSketchParametersDelegate& pd );

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const LinkedAngleConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};


} // namespace cad
} // namespace insight

#endif // ANGLECONSTRAINT_H
