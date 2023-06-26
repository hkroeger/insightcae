#ifndef IQVTKPOINTONCURVECONSTRAINT_H
#define IQVTKPOINTONCURVECONSTRAINT_H


#include "iqvtkconstrainedsketchentity.h"
#include "constrainedsketchgeometry.h"

#include "constrainedsketch.h"
#include "cadfeature.h"

class IQVTKPointOnCurveConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::SketchPoint> p_;
    std::shared_ptr<insight::cad::Feature> curve_;

    IQVTKPointOnCurveConstraint(
        std::shared_ptr<insight::cad::SketchPoint> p_,
        std::shared_ptr<insight::cad::Feature> curve_ );

public:
    declareType("PointOnCurveConstraint");

    CREATE_FUNCTION(IQVTKPointOnCurveConstraint);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;

    static void addParserRule(insight::cad::ConstrainedSketchGrammar& ruleset, insight::cad::MakeDefaultGeometryParametersFunction mdpf);

    std::set<std::comparable_weak_ptr<ConstrainedSketchEntity> > dependencies() const override;

    void replaceDependency(
        const std::weak_ptr<ConstrainedSketchEntity>& entity,
        const std::shared_ptr<ConstrainedSketchEntity>& newEntity) override;
};


#endif // IQVTKPOINTONCURVECONSTRAINT_H
