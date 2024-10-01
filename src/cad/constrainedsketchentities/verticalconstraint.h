#ifndef VERTICALCONSTRAINT_H
#define VERTICALCONSTRAINT_H

#include "constrainedsketchentity.h"
#include "cadfeatures/line.h"

namespace insight
{
namespace cad
{

class VerticalConstraint
    : public ConstrainedSketchEntity
{
    std::shared_ptr<Line> line_;

    VerticalConstraint(
        std::shared_ptr<Line> line,
        const std::string& layerName = std::string() );

public:
    declareType("VerticalConstraint");

    CREATE_FUNCTION(VerticalConstraint);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        ConstrainedSketchScriptBuffer& script,
        const std::map<const ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        ConstrainedSketchGrammar& ruleset,
        MakeDefaultGeometryParametersFunction mdpf );

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const VerticalConstraint& other);

    ConstrainedSketchEntityPtr clone() const override;
};

}
}

#endif // VERTICALCONSTRAINT_H
