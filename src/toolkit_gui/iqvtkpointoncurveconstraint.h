#ifndef IQVTKPOINTONCURVECONSTRAINT_H
#define IQVTKPOINTONCURVECONSTRAINT_H


#include "iqvtkconstrainedsketchentity.h"

#include "sketch.h"
#include "cadfeature.h"

class IQVTKPointOnCurveConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::SketchPoint const> p_;
    std::shared_ptr<insight::cad::Feature const> curve_;
public:
    IQVTKPointOnCurveConstraint(
        std::shared_ptr<insight::cad::SketchPoint const> p_,
        std::shared_ptr<insight::cad::Feature const> curve_ );

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


#endif // IQVTKPOINTONCURVECONSTRAINT_H
