#ifndef FIXEDPOINTCONSTRAINT_H
#define FIXEDPOINTCONSTRAINT_H

#include "constrainedsketchentity.h"
#include "constrainedsketch.h"

namespace insight
{
namespace cad
{

class FixedPointConstraint
    : public ConstrainedSketchEntity
{

    SketchPointPtr p_;

    FixedPointConstraint(
        SketchPointPtr p, const std::string& layerName=std::string()  );

public:
    declareType("FixedPoint");

    CREATE_FUNCTION(FixedPointConstraint);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

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
    void operator=(const FixedPointConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};

}
}

#endif // FIXEDPOINTCONSTRAINT_H
