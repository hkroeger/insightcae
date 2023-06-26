#ifndef IQVTKHORIZONTALCONSTRAINT_H
#define IQVTKHORIZONTALCONSTRAINT_H

#include "iqvtkconstrainedsketchentity.h"

#include "constrainedsketchgeometry.h"
#include "cadfeatures/line.h"

class IQVTKHorizontalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line> line_;

    IQVTKHorizontalConstraint(std::shared_ptr<insight::cad::Line> line);

public:
    declareType("HorizontalConstraint");

    CREATE_FUNCTION(IQVTKHorizontalConstraint);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(
        insight::cad::ConstrainedSketchGrammar& ruleset,
        insight::cad::MakeDefaultGeometryParametersFunction mdpf );

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;
};


#endif // IQVTKHORIZONTALCONSTRAINT_H
