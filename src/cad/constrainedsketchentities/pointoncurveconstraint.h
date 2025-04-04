#ifndef POINTONCURVECONSTRAINT_H
#define POINTONCURVECONSTRAINT_H


#include "singlesymbolconstraint.h"

#include "constrainedsketch.h"
#include "cadfeature.h"

namespace insight
{
namespace cad
{

class PointOnCurveConstraint
    : public SingleSymbolConstraint
{
    std::shared_ptr<SketchPoint> p_;
    std::shared_ptr<Feature> curve_;

    PointOnCurveConstraint(
        std::shared_ptr<SketchPoint> p,
        std::shared_ptr<Feature> curve,
        const std::string& layerName = std::string() );

public:
    declareType("PointOnCurveConstraint");

    CREATE_FUNCTION(PointOnCurveConstraint);

    std::string symbolText() const override;
    arma::mat symbolLocation() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        const ConstrainedSketchParametersDelegate& pd);

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const PointOnCurveConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};

}
}

#endif // POINTONCURVECONSTRAINT_H
