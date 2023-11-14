#ifndef IQVTKFIXEDPOINT_H
#define IQVTKFIXEDPOINT_H

#include "iqvtkconstrainedsketchentity.h"
#include "constrainedsketch.h"

class IQVTKFixedPoint
    : public IQVTKConstrainedSketchEntity
{

    insight::cad::SketchPointPtr p_;

    IQVTKFixedPoint(
        insight::cad::SketchPointPtr p, const std::string& layerName=std::string()  );

public:
    declareType("FixedPoint");

    CREATE_FUNCTION(IQVTKFixedPoint);

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

    void operator=(const ConstrainedSketchEntity& other) override;
    void operator=(const IQVTKFixedPoint& other);
};


#endif // IQVTKFIXEDPOINT_H
