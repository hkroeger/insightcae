#ifndef DISTANCECONSTRAINT_H
#define DISTANCECONSTRAINT_H

#include "constrainedsketchentity.h"
#include "cadpostprocactions/pointdistance.h"

namespace insight {
namespace cad {


class DistanceConstraint
    : public Distance,
      public ConstrainedSketchEntity
{
    VectorPtr planeNormal_;

    size_t calcHash() const override;


protected:
    DistanceConstraint(
        VectorPtr p1, VectorPtr p2,
        VectorPtr planeNormal,
        const std::string& layerName = std::string(),
        VectorPtr distanceAlong = VectorPtr());

public:
    declareType("DistanceConstraintBase");

    virtual double targetValue() const =0;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;


    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;
    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    arma::mat dimLineOffset() const override;
    void setDimLineOffset(const arma::mat& p);
    double relativeArrowSize() const override;

    VectorPtr planeNormal() const;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const DistanceConstraint& other);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;
};




class FixedDistanceConstraint
    : public DistanceConstraint
{

    FixedDistanceConstraint(
        VectorPtr p1, VectorPtr p2, VectorPtr planeNormal,
        const std::string& layerName = std::string(),
        VectorPtr distanceAlong=VectorPtr() );

public:
    declareType("DistanceConstraint");

    CREATE_FUNCTION(FixedDistanceConstraint);

    double targetValue() const override;
    void setTargetValue(double dist);

    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        const ConstrainedSketchParametersDelegate& pd);

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const FixedDistanceConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};




class LinkedDistanceConstraint
    : public DistanceConstraint
{

    std::string distExpr_;
    ScalarPtr distance_;


    LinkedDistanceConstraint(
        VectorPtr p1, VectorPtr p2,
        ScalarPtr dist, VectorPtr planeNormal,
        const std::string& layerName = std::string(),
        const std::string& distExpr = std::string(),
        VectorPtr distanceAlong=VectorPtr() );

public:
    declareType("Distance");

    CREATE_FUNCTION(LinkedDistanceConstraint);

    double targetValue() const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        const ConstrainedSketchParametersDelegate& pd );

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const LinkedDistanceConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};



} // namespace cad
} // namespace insight

#endif // DISTANCECONSTRAINT_H
