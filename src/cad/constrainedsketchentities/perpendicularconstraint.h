#ifndef PERPENDICULARCONSTRAINT_H
#define PERPENDICULARCONSTRAINT_H

#include "singlesymbolconstraint.h"
#include "cadfeatures/singleedgefeature.h"


namespace insight
{
namespace cad
{

class PerpendicularConstraint
    : public SingleSymbolConstraint
{
    std::shared_ptr<SingleEdgeFeature> line1_, line2_;

    enum Attachment { Start, End };

    std::pair<Attachment,Attachment> commonPoint() const;

    PerpendicularConstraint(
        std::shared_ptr<SingleEdgeFeature> line1,
        std::shared_ptr<SingleEdgeFeature> line2,
        const std::string& layerName = std::string());

public:
    declareType("PerpendicularConstraint");

    CREATE_FUNCTION(PerpendicularConstraint);

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
        const ConstrainedSketchParametersDelegate& pd );

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;


    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const PerpendicularConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};


}
}

#endif // PERPENDICULARCONSTRAINT_H
