#ifndef TANGENTCONSTRAINT_H
#define TANGENTCONSTRAINT_H

#include "singlesymbolconstraint.h"
#include "cadfeatures/singleedgefeature.h"


namespace insight
{
namespace cad
{

class TangentConstraint
    : public SingleSymbolConstraint
{
    std::shared_ptr<SingleEdgeFeature> line1_, line2_;

    enum Attachment { Start, End };

    std::pair<Attachment,Attachment> commonPoint() const;

    TangentConstraint(
        std::shared_ptr<SingleEdgeFeature> line1,
        std::shared_ptr<SingleEdgeFeature> line2,
        const std::string& layerName = std::string());

public:
    declareType("TangentConstraint");

    CREATE_FUNCTION(TangentConstraint);

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
    void operator=(const TangentConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};


}
}

#endif // TANGENTCONSTRAINT_H
