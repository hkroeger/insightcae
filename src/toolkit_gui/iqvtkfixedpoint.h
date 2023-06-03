#ifndef IQVTKFIXEDPOINT_H
#define IQVTKFIXEDPOINT_H

#include "iqvtkconstrainedsketchentity.h"
#include "sketch.h"

class IQVTKFixedPoint
    : public IQVTKConstrainedSketchEntity
{

    insight::cad::SketchPointPtr p_;

public:
    IQVTKFixedPoint(
        insight::cad::SketchPointPtr p  );

    std::vector<vtkSmartPointer<vtkProp> > createActor() const override;

    int nConstraints() const override;
    double getConstraintError(unsigned int iConstraint) const override;

    void scaleSketch(double scaleFactor) override;

    void generateScriptCommand(
        insight::cad::ConstrainedSketchScriptBuffer& script,
        const std::map<const insight::cad::ConstrainedSketchEntity*, int>& entityLabels) const override;
};


#endif // IQVTKFIXEDPOINT_H
