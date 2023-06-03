#ifndef IQVTKVERTICALCONSTRAINT_H
#define IQVTKVERTICALCONSTRAINT_H

#include "iqvtkconstrainedsketchentity.h"
#include "cadfeatures/line.h"


class IQVTKVerticalConstraint
    : public IQVTKConstrainedSketchEntity
{
    std::shared_ptr<insight::cad::Line const> line_;
public:
    IQVTKVerticalConstraint(std::shared_ptr<insight::cad::Line const> line);

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;
    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};

#endif // IQVTKVERTICALCONSTRAINT_H
